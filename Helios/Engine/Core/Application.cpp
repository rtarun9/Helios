#include "Pch.hpp"

#include "Core/Application.hpp"
#include "Core/Engine.hpp"

#include "imgui_impl_win32.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace helios
{
	int Application::Run(Engine* engine, HINSTANCE instance)
	{
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
			.lpszClassName = WINDOW_CLASS_NAME.data(),
			.hIconSm = nullptr
		};

		if (!(::RegisterClassExW(&windowClass)))
		{
			ErrorMessage(L"Failed to register window class");
		}

		s_WindowRect = 
		{
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(engine->GetWidth()),
			.bottom = static_cast<LONG>(engine->GetHeight())
		};

		::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		::AdjustWindowRect(&s_WindowRect, WS_OVERLAPPEDWINDOW, FALSE);

		s_ClientWidth = s_WindowRect.right - s_WindowRect.left;
		s_ClientHeight = s_WindowRect.bottom - s_WindowRect.top;

		// Get Screen width and height so as to center the window.
		int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

		// Clamp value of client region so that it does not exceed the screen width / height.
		s_ClientWidth = std::clamp<uint32_t>(s_ClientWidth, 0, screenWidth);
		s_ClientHeight = std::clamp<uint32_t>(s_ClientHeight, 0, screenHeight);

		int windowXPos = std::max<int>(0, (screenWidth - s_ClientWidth) / 2);
		int windowYPos = std::max<int>(0, (screenHeight - s_ClientHeight) / 2);

		// Pass pointer to engine as last parameter to createWindow. We can retrieve this data in the WindowProc function by reinterpreting the lParam as a LPCREATESTRUCT.
		s_WindowHandle = ::CreateWindowExW(0, WINDOW_CLASS_NAME.data(), engine->GetTitle().c_str(), WS_OVERLAPPEDWINDOW, windowXPos, windowYPos,
			s_ClientWidth, s_ClientHeight, 0, 0, instance, engine);

		::GetWindowRect(s_WindowHandle, &s_WindowRect);

		if (!s_WindowHandle)
		{
			ErrorMessage(L"Failed to create window");
		}

		engine->OnInit();

		::ShowWindow(s_WindowHandle, SW_SHOW);

		// Main game loop
		MSG message{};
		while (message.message != WM_QUIT)
		{
			s_Timer.Tick();

			if (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessageW(&message);
			}

			engine->OnUpdate();
			engine->OnRender();
		}

		engine->OnDestroy();

		::UnregisterClassW(WINDOW_CLASS_NAME.data(), instance);

		return static_cast<char>(message.wParam);
	}

	HWND Application::GetWindowHandle()
	{
		return s_WindowHandle;
	}

	uint32_t Application::GetClientWidth()
	{
		return s_ClientWidth;
	}

	uint32_t Application::GetClientHeight()
	{
		return s_ClientHeight;
	}

	RECT& Application::GetWindowRect()
	{
		return s_WindowRect;
	}

	Timer& Application::GetTimer()
	{
		return s_Timer;
	}

	void Application::ToggleFullScreenMode()
	{
		if (!s_IsFullScreen)
		{
			::GetWindowRect(s_WindowHandle, &s_WindowRect);

			// Set window style to borderless so entire screen is filled by the client region.
			UINT fullScreenWindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(s_WindowHandle, GWL_STYLE, fullScreenWindowStyle);

			// Get info of the nearest display in case of multi monior setup or primary display in single monitor setup.
			HMONITOR monitor = ::MonitorFromWindow(s_WindowHandle, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEXW monitorInfo{};
			monitorInfo.cbSize = sizeof(MONITORINFOEXW);
			::GetMonitorInfoW(monitor, &monitorInfo);

			auto width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			auto height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

			::SetWindowPos(s_WindowHandle, HWND_TOP,
				monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
				width, height, SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(s_WindowHandle, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(s_WindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			s_ClientWidth = s_WindowRect.right - s_WindowRect.left;
			s_ClientHeight = s_WindowRect.bottom - s_WindowRect.top;

			::SetWindowPos(s_WindowHandle, HWND_NOTOPMOST,
				s_WindowRect.left, s_WindowRect.top,
				s_ClientWidth, s_ClientWidth, SWP_FRAMECHANGED | SWP_NOACTIVATE);
			
			::ShowWindow(s_WindowHandle, SW_NORMAL);
		}

		s_IsFullScreen = !s_IsFullScreen;
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
					::DestroyWindow(s_WindowHandle);
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

			case WM_SIZE:
			{
				// Dont save current window dimensions while switching from FullScreen -> Normal mode.
				if (s_IsFullScreen)
				{
					::GetClientRect(s_WindowHandle, &s_WindowRect);

					s_ClientWidth = s_WindowRect.right - s_WindowRect.left;
					s_ClientHeight = s_WindowRect.bottom - s_WindowRect.top;
				}

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