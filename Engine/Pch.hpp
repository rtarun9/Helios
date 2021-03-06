#pragma once

// Exclude rarely used stuff from the Windows headers.
#define WIN32_LEAN_AND_MEAN

// Undef the Min / Max macros : prefer using std::min / std::max functions from the algorithsm header instead.
#define NOMINMAX

// Prefer W (Wide string) versions of functions / structs from WIN32 Api instead of A (Ansi string) variants.
#ifndef UNICODE
#define UNICODE
#endif

// System includes.
#include <Windows.h>

// DirectX and DXGI Includes.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include "Graphics/API/d3dx12.hpp"

// For the ComPtr<> template class.
#include <wrl/client.h>

// For converting wstring to string and vice versa.
#include <atlbase.h>
#include <atlconv.h>

// STL Includes.
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <exception>
#include <chrono>
#include <memory>
#include <array>
#include <utility>
#include <span>
#include <functional>
#include <queue>
#include <map>
#include <type_traits>
#include <stdexcept>
#include <variant>
#include <thread>

#include "Core/Helpers.hpp"

// To be used only in the .cpp files.
namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

// For setting the Agility SDK paramters.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 602u; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
