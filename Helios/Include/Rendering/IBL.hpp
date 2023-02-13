#pragma once

#include "../Graphics/PipelineState.hpp"
#include "../Graphics/Resources.hpp"

namespace helios::gfx
{
    class GraphicsDevice;
}

namespace helios::rendering
{
    // Abstraction responsible of creating IBL related textures given a cube map (i.e the diffuse irradiance texture,
    // specular prefilter texture and BRDF LUT texture).
    class IBL
    {
      public:
        // Create the required pipeline states.
        IBL(gfx::GraphicsDevice* const graphicsDevice);

        [[nodiscard]] gfx::Texture generateIrradianceTexture(gfx::GraphicsDevice* const graphicsDevice,
                                                             const gfx::Texture& cubeMapTexture);

        [[nodiscard]] gfx::Texture generatePrefilterTexture(gfx::GraphicsDevice* const graphicsDevice,
                                                            const gfx::Texture& cubeMapTexture);

        [[nodiscard]] gfx::Texture generateBRDFLutTexture(gfx::GraphicsDevice* const graphicsDevice);

      private:
        static constexpr uint32_t IRRADIANCE_MAP_TEXTURE_DIMENSION = 64u;
        static constexpr uint32_t PREFILTER_MAP_TEXTURE_DIMENSION = 1024u;
        static constexpr uint32_t BRDF_LUT_TEXTURE_DIMENSION = 32u;

        gfx::PipelineState m_brdfLUTPipelineState{};
        gfx::PipelineState m_irradianceConvolutionPipelineState{};
        gfx::PipelineState m_prefilterConvolutionPipelineState{};
    };
} // namespace helios::rendering