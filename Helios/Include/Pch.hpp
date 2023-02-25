#pragma once

// STL includes.
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <iostream>
#include <ranges>
#include <source_location>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <future>
#include <queue>
#include <vector>

// Win32 / DirectX12 / DXGI includes.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <D3D12MemAlloc.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Graphics/d3dx12.hpp"

// Custom includes.
#include "Utils.hpp"

// Namespace aliases.
namespace wrl = Microsoft::WRL;
namespace math = DirectX;


// Setup constexpr indicating which build configuration was used.
#ifdef _DEBUG
constexpr bool HELIOS_DEBUG_MODE = true;
#else
constexpr bool HELIOS_DEBUG_MODE = false;
#endif

// Global variables.
constexpr uint32_t INVALID_INDEX_U32 = 0xFFFFFFFF;