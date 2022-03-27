#pragma once

#include "Pch.hpp"

#include <stdexcept>

static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
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