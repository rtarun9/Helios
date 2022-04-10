#pragma once

#include "Pch.hpp"

#include <stdexcept>

static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		char hrStrValue[64]{};
		sprintf_s(hrStrValue, "\nHRESULT of 0x%08X\n", static_cast<UINT>(hr));
		OutputDebugStringA(hrStrValue);

		throw std::exception(hrStrValue);
	}
}

static inline void ErrorMessage(std::wstring_view message)
{
	MessageBoxW(nullptr, message.data(), L"Error!", MB_OK);
	exit(EXIT_FAILURE);
}

static inline void GetBlobMessage(ID3DBlob* blob)
{
	if (blob)
	{
		auto message = (const char*)blob->GetBufferPointer();
		OutputDebugStringA(message);
	}
}

static inline constexpr std::string WstringToString(std::wstring_view inputWString)
{
	std::string result{ begin(inputWString), end(inputWString) };
	return result;
}

static inline constexpr std::wstring StringToWString(std::string_view inputWString)
{
	std::wstring result{ begin(inputWString), end(inputWString) };
	return result;
}


static enum class Keys : uint8_t
{
	W,
	A,
	S,
	D,
	AUp,
	ALeft,
	ADown,
	ARight,
	TotalKeyCount
};

// TODO : See a way to make this constexpr (the EnumClassValue function is constexpr, but making INPUT_MAP also constexpr would be preferrable).
static std::map<uint8_t, Keys> INPUT_MAP 
{
	{'W', Keys::W},
	{'A', Keys::A},
	{'S', Keys::S},
	{'D', Keys::D},
	{VK_UP, Keys::AUp},
	{VK_LEFT, Keys::ALeft},
	{VK_DOWN, Keys::ADown},
	{VK_RIGHT, Keys::ARight}
};

template <typename T>
static constexpr std::underlying_type<T>::type EnumClassValue(const T& value)
{
	return static_cast<std::underlying_type<T>::type>(value);
}

// For Use with Win32 : Take in a RECT and return client region width and height.
static std::pair<uint32_t, uint32_t> GetClientRegionDimentions(RECT rect, DWORD style = WS_OVERLAPPEDWINDOW)
{
	uint32_t width = static_cast<uint32_t>(rect.right - rect.left);
	uint32_t height = static_cast<uint32_t>(rect.bottom - rect.top);

	return { width, height };
}

static std::pair<uint32_t, uint32_t> GetMonitorDimensions(MONITORINFOEXW& monitorInfo)
{
	uint32_t width = static_cast<uint32_t>(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
	uint32_t height = static_cast<uint32_t>(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);

	return { width, height };
}