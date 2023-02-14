#pragma once

#include "../Graphics/PipelineState.hpp"
#include "../Graphics/Resources.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{
    class Scene;
}

namespace helios::rendering
{
    // Renders the scene from the point of viwe of the directional light source to obtain a depth map that was created
    // by rendering the scene from the POV of the directional light source.
    class PCFShadowMappingPass
    {
      public:
        // Create the required pipeline state, buffers and textures.
        PCFShadowMappingPass(gfx::GraphicsDevice* const graphicsDevice);

        // Note : the directional light is always at index 0 of the light buffer.
        void render(scene::Scene* const scene, gfx::GraphicsContext* const graphicsContext);

      public:
        static constexpr uint32_t SHADOW_MAP_DIMENSIONS = 2048u;

        gfx::PipelineState m_shadowPassPipelineState{};
        gfx::Texture m_shadowDepthBuffer{};

        gfx::Buffer m_shadowBuffer{};
        interlop::ShadowBuffer m_shadowBufferData{};
    };
} // namespace helios::rendering