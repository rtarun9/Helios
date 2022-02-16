#include "Pch.hpp"

#include "Application.hpp"
#include "Engine.hpp"

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
			.lpszClassName = WINDOW_CLASS_NAME,
			.hIconSm = nullptr
		};

		if (!(::RegisterClassExW(&windowClass)))
		{
			ErrorMessage(L"Failed to register window class");
		}

		RECT windowRect
		{
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(engine->GetWidth()),
			.bottom = static_cast<LONG>(engine->GetHeight())
		};

		::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		s_ClientWidth = windowRect.right - windowRect.left;
		s_ClientHeight = windowRect.bottom - windowRect.top;

		// Get Screen width and height so as to center the window.
		int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

		// Clamp value of client region so that it does not exceed the screen width / height.
		s_ClientWidth = std::clamp<uint32_t>(s_ClientWidth, 0, screenWidth);
		s_ClientHeight = std::clamp<uint32_t>(s_ClientHeight, 0, screenHeight);

		int windowXPos = std::max<int>(0, (screenWidth - s_ClientWidth) / 2);
		int windowYPos = std::max<int>(0, (screenHeight - s_ClientHeight) / 2);

		// Pass pointer to engine as last parameter to createWindow. We can retrieve this data in the WindowProc function by reinterpreting the lParam as a LPCREATESTRUCT.
		s_WindowHandle = ::CreateWindowExW(0, WINDOW_CLASS_NAME, engine->GetTitle().c_str(), WS_OVERLAPPEDWINDOW, windowXPos, windowYPos,
			s_ClientWidth, s_ClientHeight, 0, 0, instance, engine);

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
			if (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessageW(&message);
			}

			engine->OnUpdate();
			engine->OnRender();
		}

		engine->OnDestroy();

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

	LRESULT CALLBACK Application::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Engine* engine= reinterpret_cast<Engine*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

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
				engine->OnKeyDown(static_cast<uint8_t>(wParam));


				if (wParam == VK_ESCAPE)
				{
					::DestroyWindow(s_WindowHandle);
				}

				break;
			}

			case WM_KEYUP:
			{
				engine->OnKeyUp(static_cast<uint8_t>(lParam));
				break;
			}

			case WM_DESTROY:
			{
				::PostQuitMessage(0);
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