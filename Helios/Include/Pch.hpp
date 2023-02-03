#pragma once

#include <chrono>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <span>
#include <ranges>
#include <vector>
#include <array>
#include <filesystem>
#include <format>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include <d3dcompiler.h>

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