#pragma once

#include "ComputeContext.hpp"
#include "PipelineState.hpp"
#include "Resources.hpp"

namespace helios::gfx
{
    // The device abstraction will have an object of this type.
    // Handles generation of mip maps for a given texture.
    class MipMapGenerator
    {
      public:
        explicit MipMapGenerator(GraphicsDevice* const graphicsDevice);

        void generateMips(Texture& texture);

      private:
        PipelineState m_mipMapPipelineState{};
        Buffer m_mipMapBuffer{};

        GraphicsDevice& graphicsDevice;
    };

} // namespace helios::gfx
