#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <iostream>
#include <ranges>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <future>
#include <vector>

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

namespace wrl = Microsoft::WRL;
namespace math = DirectX;

#include "Utils.hpp"

// Setup constexpr indicating which build configuration was used.
#ifdef _DEBUG
constexpr bool HELIOS_DEBUG_MODE = true;
#else
constexpr bool HELIOS_DEBUG_MODE = false;
#endif

constexpr uint32_t INVALID_INDEX_U32 = 0xFFFFFFFF;