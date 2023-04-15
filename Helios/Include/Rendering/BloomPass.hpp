#pragma once

#include "../Graphics/PipelineState.hpp"
#include "../Graphics/Resources.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"

// Bloom effect inspired (extremely heavily) from the Call Of Duty Advanced Warfare presentation on :
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare

namespace helios::gfx
{
    class GraphicsContext;
    class GraphicsDevice;
    class ComputeContext;
} // namespace helios::gfx

namespace helios::rendering
{
    class BloomPass
    {
      public:
        BloomPass(gfx::GraphicsDevice* const graphicsDevice, const uint32_t width, const uint32_t height);

        void render(gfx::GraphicsContext* const graphicsContext, gfx::Texture& shadingTexture, gfx::Texture& lightPassTexture, const uint32_t width,
                    const uint32_t height);

      public:
        gfx::Texture m_bloomDownSampleTexture{};
        gfx::PipelineState m_bloomDownSamplePipelineState{};

        gfx::Texture m_bloomUpSampleTexture{};
        gfx::PipelineState m_bloomUpSamplePipelineState{};

        gfx::Texture m_extractionTexture{};
        gfx::PipelineState m_extractionPipelineState{};

        gfx::Buffer m_bloomBuffer{};
        interlop::BloomBuffer m_bloomBufferData{};
    };
} // namespace helios::rendering