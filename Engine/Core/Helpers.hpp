#pragma once

#include "Pch.hpp"

// Reference : https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/DXSampleHelper.h.


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

static inline std::string HresultToString(HRESULT hr)
{
	char str[128]{};
	sprintf_s(str, "HRESULT : 0x%08X", static_cast<UINT>(hr));
	return std::string(str);
}

static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception(HresultToString(hr).c_str());
		exit(EXIT_FAILURE);
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

#define CREATE_LAMBDA_FUNCTION(function) ([&](){function;})

// Deferred execution queue for templated functions.
struct DeferredExecutionQueue
{
	std::vector<std::function<void()>> functionPointers;

	void PushFunction(std::function<void()>&& function)
	{
		functionPointers.push_back(function);
	}

	void Execute()
	{
		for (auto it = functionPointers.rbegin(); it != functionPointers.rend(); it++)
		{
			(*it)();
		}

		functionPointers.clear();
	}
};

template <typename T>
static constexpr std::underlying_type<T>::type EnumClassValue(const T& value)
{
	return static_cast<std::underlying_type<T>::type>(value);
}

// Used for window dimensions.
struct Uint2
{
	uint32_t x{};
	uint32_t y{};

	auto operator<=>(Uint2 const& other) const = default;
};

static Uint2 GetClientRegionDimentions(RECT rect, DWORD style = WS_OVERLAPPEDWINDOW)
{
	uint32_t width = static_cast<uint32_t>(rect.right - rect.left);
	uint32_t height = static_cast<uint32_t>(rect.bottom - rect.top);

	return  Uint2{.x =  width, .y = height };
}

static Uint2 GetMonitorDimensions(MONITORINFOEXW& monitorInfo)
{
	uint32_t width = static_cast<uint32_t>(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
	uint32_t height = static_cast<uint32_t>(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);

	return Uint2{ .x = width, .y = height };
}

