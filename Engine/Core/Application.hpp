#pragma once

#include "Pch.hpp"

#include "Timer.hpp"

namespace helios
{
	class Engine;

	// The Application Static Class holds the window and has the main game loop.
	// It communicates with the engine via the OnUpdate(), OnRender() etc function.
	class Application
	{
	public:
		static int Run(Engine* engine, HINSTANCE instance);
		
		static HWND GetWindowHandle() { return s_WindowHandle; };
		static uint32_t GetClientWidth() { return s_ClientWidth; }
		static uint32_t GetClientHeight() { return s_ClientHeight; }

		static RECT& GetWindowRect() { return s_WindowRect; };

		static Timer& GetTimer() { return s_Timer; };

	private:
		// The Application class is only accesible via its static member functions.
		Application() = default;

		static void ToggleFullScreenMode();

		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static constexpr inline const wchar_t* WINDOW_CLASS_NAME{L"Base Window Class"};
		
		static inline HWND s_WindowHandle{};

		static inline uint32_t s_ClientWidth{};
		static inline uint32_t s_ClientHeight{};

		static inline RECT s_WindowRect{};
		static inline bool s_IsFullScreen{ false };

		static inline Timer s_Timer{};
	};


}
