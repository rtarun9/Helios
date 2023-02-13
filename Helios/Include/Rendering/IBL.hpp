#pragma once

#include "../Graphics/Resources.hpp"
#include "../Graphics/PipelineState.hpp"

namespace helios::gfx
{
    class GraphicsDevice;
}

namespace helios::rendering
{
    // Abstraction responsible of creating IBL related textures given a cube map (i.e the diffuse irradiance texture).
    class IBL
    {
      public:
        // Create the required pipeline states.
        IBL(gfx::GraphicsDevice* const graphicsDevice);

        gfx::Texture generateIrradianceTexture(gfx::GraphicsDevice* const graphicsDevice,
                                               const gfx::Texture& cubeMapTexture);

      private:
        static constexpr uint32_t IRRADIANCE_MAP_TEXTURE_DIMENSION = 64u;
        static constexpr uint32_t PREFILTER_MAP_TEXTURE_DIMENSION = 1024u;
        static constexpr uint32_t BRDF_LUT_TEXTURE_DIMENSION = 256u;

        gfx::PipelineState m_irradianceConvolutionPipelineState{};
    };
} // namespace helios::rendering