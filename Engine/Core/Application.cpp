#include "Pch.hpp"

#include "Core/Application.hpp"
#include "Core/Engine.hpp"

#include "imgui_impl_win32.h"

// Forward declare message handler from imgui_impl_win32.cpp.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace helios
{
	int Application::Run(Engine* engine, HINSTANCE instance)
	{
		::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		// Initialize Window class.

		// Force window redraw when either width / height of client region changes or if movement adjustment happens.
		WNDCLASSEXW windowClass
		{
			.cbSize = sizeof(WNDCLASSEXW),
			.style = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = WindowProc,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = instance,
			.hIcon = nullptr,
			.hCursor = nullptr,
			.hbrBackground = nullptr,
			.lpszMenuName = nullptr,
			.lpszClassName = WINDOW_CLASS_NAME,
			.hIconSm = nullptr
		};

		if (!(::RegisterClassExW(&windowClass)))
		{
			ErrorMessage(L"Failed to register window class");
		}

		sWindowRect = 
		{
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(engine->GetDimensions().x),
			.bottom = static_cast<LONG>(engine->GetDimensions().y)
		};

		sClientDimensions = GetClientRegionDimentions(sWindowRect, WS_OVERLAPPEDWINDOW);

		Uint2 windowPosition = CenterWindow();

		// Pass pointer to engine as last parameter to createWindow. We can retrieve this data in the WindowProc function by reinterpreting the lParam as a LPCREATESTRUCT.
		sWindowHandle = ::CreateWindowExW(0, WINDOW_CLASS_NAME, engine->GetTitle().c_str(), WS_OVERLAPPEDWINDOW, windowPosition.x, windowPosition.y,
			sClientDimensions.x, sClientDimensions.y, 0, 0, instance, engine);

		::GetClientRect(sWindowHandle, &sWindowRect);
		sClientDimensions = GetClientRegionDimentions(sWindowRect, WS_OVERLAPPEDWINDOW);

		if (!sWindowHandle)
		{
			ErrorMessage(L"Failed to create window");
		}

		engine->OnInit();

		if (sWindowHandle)
		{
			::ShowWindow(sWindowHandle, SW_SHOW);
		}

		// Main game loop
		MSG message{};
		while (message.message != WM_QUIT)
		{
			sTimer.Tick();

			if (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessageW(&message);
			}

			engine->OnUpdate();
			engine->OnRender();
		}

		engine->OnDestroy();

		::UnregisterClassW(WINDOW_CLASS_NAME, instance);

		return static_cast<char>(message.wParam);
	}

	void Application::ToggleFullScreenMode()
	{
		if (!sIsFullScreen)
		{
			::GetClientRect(sWindowHandle, &sWindowRect);

			// Set window style to borderless so entire screen is filled by the client region. The full screen window style is basically 0ed out by these flag's, done here explicitly.
			UINT fullScreenWindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			::SetWindowLongW(sWindowHandle, GWL_STYLE, fullScreenWindowStyle);

			// Get info of the nearest display in case of multi monior setup or primary display in single monitor setup.
			HMONITOR monitor = ::MonitorFromWindow(sWindowHandle, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEXW monitorInfo{};
			monitorInfo.cbSize = sizeof(MONITORINFOEXW);
			::GetMonitorInfoW(monitor, &monitorInfo);

			auto [width, height] = GetMonitorDimensions(monitorInfo);

			::SetWindowPos(sWindowHandle, HWND_TOP,
				monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
				width, height, SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(sWindowHandle, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(sWindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::GetClientRect(sWindowHandle, &sWindowRect);
			sClientDimensions = GetClientRegionDimentions(sWindowRect);

			::SetWindowPos(sWindowHandle, HWND_NOTOPMOST,
				sWindowRect.left, sWindowRect.top,
				sClientDimensions.x, sClientDimensions.y, SWP_FRAMECHANGED | SWP_NOACTIVATE);
			
			::ShowWindow(sWindowHandle, SW_NORMAL);
		}

		sIsFullScreen = !sIsFullScreen;
	}

	Uint2 Application::CenterWindow()
	{
		// Get Screen width and height so as to center the window.
		int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

		// Clamp value of client region so that it does not exceed the screen width / height.
		sClientDimensions.x = std::clamp<uint32_t>(sClientDimensions.x, 0, screenWidth);
		sClientDimensions.y = std::clamp<uint32_t>(sClientDimensions.y, 0, screenHeight);

		uint32_t windowXPos = std::max<uint32_t>(0, (screenWidth - sClientDimensions.x) / 2);
		uint32_t windowYPos = std::max<uint32_t>(0, (screenHeight - sClientDimensions.y) / 2);

		return Uint2{ .x = windowXPos, .y = windowYPos };
	}

	LRESULT CALLBACK Application::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Engine* engine= reinterpret_cast<Engine*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

		// Handle ImGUI messages.
		if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
		{
			return true;
		}

		switch (message)
		{
			case WM_CREATE:
			{
				// Save the Engine* passed in to CreateWindow.
				LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
				SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
				break;
			}

			case WM_KEYDOWN:
			{
				engine->OnKeyAction(static_cast<uint8_t>(wParam), true);


				if (wParam == VK_ESCAPE)
				{
					::DestroyWindow(sWindowHandle);
				}

				if (wParam == VK_F11)
				{
					ToggleFullScreenMode();
				}

				break;
			}

			case WM_KEYUP:
			{
				engine->OnKeyAction(static_cast<uint8_t>(wParam), false);
				break;
			}

			case WM_DESTROY:
			{
				::PostQuitMessage(0);
				break;
			}

			// note(rtarun9) : TODO : FIX RESIZE ISSUE WITH IMGUI, CHECK CODE LOGIC FOR RESIZING.
			case WM_SIZE:
			{
				// Dont save current window dimensions while switching from FullScreen -> Normal mode.
				if (sIsFullScreen)
				{
					::GetClientRect(sWindowHandle, &sWindowRect);
					sClientDimensions = GetClientRegionDimentions(sWindowRect);

				}

				engine->OnResize();

				break;
			}

			default:
			{
				break;
			}
		}

		// Handle any messages the switch statement didn't.
		return ::DefWindowProc(windowHandle, message, wParam, lParam);
	}
}