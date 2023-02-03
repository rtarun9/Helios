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
    namespace ShaderCompiler
    {
        [[nodiscard]] Shader compile(const ShaderTypes& shaderType, const std::wstring_view shaderPath, const std::wstring_view entryPoint, const bool extractRootSignature = false);
    } // namespace ShaderCompiler
} // namespace helios::gfx