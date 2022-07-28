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
		
		static inline HWND GetWindowHandle() { return sWindowHandle; }

		static inline Uint2 GetClientDimensions() { return sClientDimensions; }

		static inline Uint2 GetFullScreenDimensions() { return sFullScreenClientDimensions; }

		static inline RECT& GetWindowRect() { return sWindowRect; }


		static inline Timer& GetTimer() { return sTimer; }

		static bool IsFullScreen() { return sIsFullScreen; }
	private:
		// The Application class is only accesible via its static member functions.
		Application() = default;

		static void ToggleFullScreenMode();

		// Returns the x and y coords of the top left corner of window where if placed the window will be centered.
		static Uint2 CenterWindow();

		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static constexpr inline const wchar_t* WINDOW_CLASS_NAME{L"Base Window Class"};
		
		static inline HWND sWindowHandle{};

		static inline Uint2 sClientDimensions{};

		// Required to tell the UI manager the display region if window is full screen.
		static inline Uint2 sFullScreenClientDimensions{};

		static inline RECT sWindowRect{};
	

		static inline bool sIsFullScreen{ false };

		static inline Timer sTimer{};
	};


}
