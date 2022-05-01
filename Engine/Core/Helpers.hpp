#pragma once

#include "Pch.hpp"

static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception("Failed HRESULT.");
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

static inline std::string WstringToString(std::wstring_view inputWString) 
{
	std::string result{};
	std::wstring wideStringData = inputWString.data();
	CW2A cw2a(wideStringData.c_str());
	return std::string(cw2a);
}

static inline std::wstring StringToWString(std::string_view inputString)
{
	std::wstring result{};
	std::string stringData = inputString.data();
	CA2W ca2w(stringData.c_str());
	return std::wstring(ca2w);
}

template <typename T>
static constexpr std::underlying_type<T>::type EnumClassValue(const T& value)
{
	return static_cast<std::underlying_type<T>::type>(value);
}

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
