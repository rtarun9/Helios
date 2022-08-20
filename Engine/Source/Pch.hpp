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
#include <d3dcompiler.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include "Graphics/API/d3dx12.hpp"

// SIMD Math library.
#include <DirectXMath.h>

// D3D12 memory allocator (will be removed eventually in project when custom allocator is used).
#include "D3D12MemAlloc.h"

// For the ComPtr<> template class.
#include <wrl/client.h>

// For converting wstring to string and vice versa.
#include <atlbase.h>
#include <atlconv.h>

// STL Includes.
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Utility/Helpers.hpp"
#include "Utility/ResourceManager.hpp"

// To be used only in the .cpp files.
namespace wrl = Microsoft::WRL;
namespace math = DirectX;
