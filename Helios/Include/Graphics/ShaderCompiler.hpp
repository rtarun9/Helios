#pragma once

#include "Resources.hpp"

namespace helios::gfx
{
    enum class ShaderTypes : uint8_t
    {
        Vertex,
        Pixel,
        Compute,
        RootSignature,
    };

    // Rather than using a static class, a name space is used here. The corresponding .cpp file will hold the 'member
    // functions' of the name space.
    // The DirectX Shader Compiler C++ API is used for shader compilation. 
    namespace ShaderCompiler
    {
        // The ShaderPath passed in is 'absolute' (i.e relative to the executable and not the root directory). This is because this function isn't really meant
        // to be used from the application side, as pipeline state creation functions and the ResourceManager will handle resource creation.
        [[nodiscard]] Shader compile(const ShaderTypes& shaderType, const std::wstring_view shaderPath, const std::wstring_view entryPoint, const bool extractRootSignature = false);
    } // namespace ShaderCompiler
} // namespace helios::gfx