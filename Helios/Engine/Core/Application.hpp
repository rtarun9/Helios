#pragma once

#include "Pch.hpp"

namespace helios
{
	class Engine;

	class Application
	{
	public:
		static int Run(Engine* engine, HINSTANCE instance);
		
		static HWND GetWindowHandle();
		static uint32_t GetClientWidth();
		static uint32_t GetClientHeight();

	private:
		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static constexpr wchar_t WINDOW_CLASS_NAME[] = L"Base Window Class";
		
		static inline HWND s_WindowHandle{};

		static inline uint32_t s_ClientWidth{};
		static inline uint32_t s_ClientHeight{};
	};


}
