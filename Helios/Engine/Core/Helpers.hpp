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

static inline constexpr std::string WstringToString(std::wstring_view inputWString)
{
	std::string result{ begin(inputWString), end(inputWString) };
	return result;
}