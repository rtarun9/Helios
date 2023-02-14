#pragma once

#include "Model.hpp"

#include "../Graphics/PipelineState.hpp"

#include "ShaderInterlop/RenderResources.hlsli"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{

    struct CubeMapCreationDesc
    {
        std::wstring equirectangularTexturePath{};
        std::wstring name{};
    };

    // CubeMap abstraction handles creation of TextureCube from equirectangular texture and will also provide functions for rendering the cube map (it
    // stores a Cube Model).
    class CubeMap
    {
      public:
        CubeMap(gfx::GraphicsDevice* const graphicsDevice, const CubeMapCreationDesc& cubeMapCreationDesc);

        void render(const gfx::GraphicsContext* const graphicsContext,
                    interlop::CubeMapRenderResources& renderResources);

      public:
        static constexpr uint32_t CUBEMAP_TEXTURE_DIMENSION = 1024u;
       
        std::unique_ptr<scene::Model> m_cubeModel{};

        gfx::PipelineState m_cubeMapPipelineState{};
        gfx::PipelineState m_equirectTextureToCubeMapPipelineState{};

        gfx::Texture m_cubeMapTexture{};
    };
} // namespace helios::scene