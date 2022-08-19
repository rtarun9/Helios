#include "Core/Application.hpp"
#include "Core/Engine.hpp"

#include "imgui_impl_win32.h"

// Forward declare message handler from imgui_impl_win32.cpp.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace helios::core
{
    int Application::Run(Engine *engine, HINSTANCE instance)
    {
        ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // Initialize Window class.

        // Force window redraw when either width / height of client region changes or if movement adjustment happens.
        WNDCLASSEXW windowClass{.cbSize = sizeof(WNDCLASSEXW),
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
                                .hIconSm = nullptr};

        if (!(::RegisterClassExW(&windowClass)))
        {
            ErrorMessage(L"Failed to register window class");
        }

        sWindowRect = {.left = 0,
                       .top = 0,
                       .right = static_cast<LONG>(engine->GetDimensions().x),
                       .bottom = static_cast<LONG>(engine->GetDimensions().y)};

        Uint2 windowPosition = CenterWindow();

        // Pass pointer to engine as last parameter to createWindow. We can retrieve this data in the WindowProc
        // function by reinterpreting the lParam as a LPCREATESTRUCT.
        sWindowHandle =
            ::CreateWindowExW(0, WINDOW_CLASS_NAME, engine->GetTitle().c_str(), WS_OVERLAPPEDWINDOW, windowPosition.x,
                              windowPosition.y, sClientDimensions.x, sClientDimensions.y, 0, 0, instance, engine);

        sClientDimensions = GetDimensionFromRect(sWindowRect);
        sPreviousWindowRect = sWindowRect;

        if (!sWindowHandle)
        {
            ErrorMessage(L"Failed to create window");
        }

        engine->OnInit();

        if (sWindowHandle)
        {
            // ShowWindow(sWindowHandle, SW_SHOW);
            Application::ToggleFullScreenMode();
        }

        // Main game loop
        bool quitLoop{false};

        MSG message{};
        while (!quitLoop)
        {
            sTimer.Tick();

            while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&message);
                ::DispatchMessageW(&message);

                if (message.message == WM_QUIT)
                {
                    quitLoop = true;
                }
            }

            if (quitLoop)
            {
                break;
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
        static bool firstToggle{true};

        // Toggling to full screen mode.
        if (!sIsFullScreen)
        {
            if (firstToggle)
            {
                sPreviousWindowRect = sWindowRect;
                firstToggle = false;
            }
            else
            {
                ::GetClientRect(sWindowHandle, &sPreviousWindowRect);
            }

            UINT fullScreenWindowStyle = 0u;
            ::SetWindowLongW(sWindowHandle, GWL_STYLE, fullScreenWindowStyle);

            // Get info of the nearest display in case of multi monior setup or primary display in single monitor setup.
            HMONITOR monitor = ::MonitorFromWindow(sWindowHandle, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEXW monitorInfo{};
            monitorInfo.cbSize = sizeof(MONITORINFOEXW);
            ::GetMonitorInfoW(monitor, &monitorInfo);

            Uint2 monitorDimensions = GetMonitorDimensions(monitorInfo);

            ::SetWindowPos(sWindowHandle, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                           monitorDimensions.x, monitorDimensions.y, SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(sWindowHandle, SW_MAXIMIZE);

            ::GetClientRect(sWindowHandle, &sWindowRect);
            sClientDimensions = GetDimensionFromRect(sWindowRect);
        }
        else
        {
            // note(rtarun9) : Currently, all window rect operations are done with the same object, which can be
            // confusing. Consider having multiple rects in the future to make things more clear.
            sWindowRect = sPreviousWindowRect;

            // Full screen to non full screen mode (revert window rect to dimensions before going to full screen mode).
            ::SetWindowLong(sWindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::AdjustWindowRect(&sWindowRect, WS_OVERLAPPEDWINDOW, FALSE);
            sClientDimensions = GetDimensionFromRect(sWindowRect);

            ::SetWindowPos(sWindowHandle, HWND_NOTOPMOST, sPreviousWindowRect.left, sPreviousWindowRect.top,
                           sClientDimensions.x, sClientDimensions.y, SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(sWindowHandle, SW_NORMAL);

            ::GetClientRect(sWindowHandle, &sWindowRect);
            sClientDimensions = GetDimensionFromRect(sWindowRect);
            sPreviousWindowRect = sWindowRect;
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

        return Uint2{.x = windowXPos, .y = windowYPos};
    }

    LRESULT CALLBACK Application::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        static bool resizingWindow{false};

        Engine *engine = reinterpret_cast<Engine *>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

        // Handle ImGUI messages.
        if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
        {
            return true;
        }

        switch (message)
        {
        case WM_CREATE: {
            // Save the Engine* passed in to CreateWindow.
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        break;

        case WM_KEYDOWN: {
            engine->OnKeyAction(static_cast<uint8_t>(wParam), true);

            if (wParam == VK_ESCAPE)
            {
                ::DestroyWindow(sWindowHandle);
            }

            if (wParam == VK_F11)
            {
                ToggleFullScreenMode();
            }
        }
        break;

        case WM_KEYUP: {
            engine->OnKeyAction(static_cast<uint8_t>(wParam), false);
        }
        break;

        case WM_DESTROY: {
            ::PostQuitMessage(0);
            return 0;
        }
        break;

        case WM_SIZE: {
            if (engine && !resizingWindow)
            {
                ::GetClientRect(sWindowHandle, &sWindowRect);
                sClientDimensions = {(UINT)LOWORD(lParam), (UINT)HIWORD(lParam)};

                engine->OnResize();
            }
        }
        break;

        case WM_ENTERSIZEMOVE: {
            resizingWindow = true;
        }
        break;

        case WM_EXITSIZEMOVE: {
            resizingWindow = false;
        }
        break;

        default: {
        }
        break;
        }

        // Handle any messages the switch statement didn't.
        return ::DefWindowProc(windowHandle, message, wParam, lParam);
    }
} // namespace helios::core