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
		
		static inline HWND GetWindowHandle() { return sWindowHandle; };
		static inline uint32_t GetClientWidth() { return sClientWidth; }
		static inline uint32_t GetClientHeight() { return sClientHeight; }

		static inline RECT& GetWindowRect() { return sWindowRect; };

		static inline Timer& GetTimer() { return sTimer; };

	private:
		// The Application class is only accesible via its static member functions.
		Application() = default;

		static void ToggleFullScreenMode();

		// Returns a pair (window x coord, window y coord) of coords to place the window's left corner at to center the window.
		static std::pair<uint32_t, uint32_t> CenterWindow();

		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static constexpr inline const wchar_t* WINDOW_CLASS_NAME{L"Base Window Class"};
		
		static inline HWND sWindowHandle{};

		static inline uint32_t sClientWidth{};
		static inline uint32_t sClientHeight{};

		static inline RECT sWindowRect{};
		static inline bool sIsFullScreen{ false };

		static inline Timer sTimer{};
	};


}
