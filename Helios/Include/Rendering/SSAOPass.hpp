#pragma once

#include "../Graphics/Resources.hpp"
#include "../Graphics/PipelineState.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::rendering
{
    // Handles SSAO (screen space ambient occlusion) pass. Will also create a blurred ssao texture.
    class SSAOPass
    {
      public:
        SSAOPass(gfx::GraphicsDevice* const graphicsDevice, const uint32_t width, const uint32_t height);

        void render(gfx::GraphicsContext* const graphicsContext, interlop::SSAORenderResources& renderResources, const uint32_t width, const uint32_t height);

      public:
        interlop::SSAOBuffer m_ssaoBufferData{};
        gfx::Buffer m_ssaoBuffer{};

        gfx::Texture m_randomRotationTexture{};

        gfx::PipelineState m_ssaoPipelineState{};
        gfx::Texture m_ssaoTexture{};

        gfx::PipelineState m_boxBlurPipelineState{};
        gfx::Texture m_blurSSAOTexture{};
    };
} // namespace helios::rendering