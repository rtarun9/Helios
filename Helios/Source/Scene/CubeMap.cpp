#include "Scene/CubeMap.hpp"

#include "Graphics/GraphicsDevice.hpp"

namespace helios::scene
{
    CubeMap::CubeMap(gfx::GraphicsDevice* const graphicsDevice, const CubeMapCreationDesc& cubeMapCreationDesc)
    {
        // Create equirectangular HDR texture and a environment cube map texture with 6 faces.

        const gfx::Texture equirectangularTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::HDRTextureFromPath,
            .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
            .mipLevels = 6,
            .bytesPerPixel = 16u,
            .name = cubeMapCreationDesc.name,
            .path = cubeMapCreationDesc.equirectangularTexturePath,
        });

        // Create cube map texture.
        m_cubeMapTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::CubeMap,
            .width = CUBEMAP_TEXTURE_DIMENSION,
            .height = CUBEMAP_TEXTURE_DIMENSION,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .mipLevels = 6u,
            .depthOrArraySize = 6u,
            .name = cubeMapCreationDesc.name + std::wstring(L"Cube Map"),
        });

        // Create compute pipeline to convert equirectangular texture to cube map.
        m_equirectTextureToCubeMapPipelineState =
            graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
                .csShaderPath = L"Shaders/CubeMap/CubeMapFromEquirectTextureCS.hlsl",
                .pipelineName = L"Equirect Texture To Cube Map",
            });

        auto computeContext = graphicsDevice->getComputeContext();
        computeContext->reset();

        computeContext->addResourceBarrier(m_cubeMapTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_COMMON,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        computeContext->executeResourceBarriers();

        computeContext->setComputeRootSignatureAndPipeline(m_equirectTextureToCubeMapPipelineState);

        uint32_t size = CUBEMAP_TEXTURE_DIMENSION;
        for (const uint32_t i : std::views::iota(0u, 6u))
        {
            const interlop::CubeFromEquirectRenderResources renderResources = {
                .textureIndex = equirectangularTexture.srvIndex,
                .outputTextureIndex = m_cubeMapTexture.uavIndex + i,
            };

            computeContext->set32BitComputeConstants(&renderResources);

            const uint32_t numGroups = std::max(1u, size / 8u);

            computeContext->dispatch(numGroups, numGroups, 6u);

            size /= 2;
        }

        computeContext->addResourceBarrier(m_cubeMapTexture.allocation.resource.Get(),
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);

        computeContext->executeResourceBarriers();

        graphicsDevice->executeAndFlushComputeContext(std::move(computeContext));

        // Generate mips for all the cube faces.
        graphicsDevice->getMipMapGenerator()->generateMips(m_cubeMapTexture);

        // Create pipeline for rendering the cube map.
        m_cubeMapPipelineState = graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/CubeMap/CubeMap.hlsl",
                    .pixelShaderPath = L"Shaders/CubeMap/CubeMap.hlsl",
                },
            .depthComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
            .frontFaceWindingOrder = gfx::FrontFaceWindingOrder::CounterClockWise,
            .pipelineName = L"Cube Map Pipeline",
        });

        // Create cube map mesh.
        m_cubeModel = std::make_unique<Model>(graphicsDevice, ModelCreationDesc{
                                                                  .modelPath = L"Assets/Models/Cube/glTF/Cube.gltf",
                                                                  .modelName = L"Cube Map",
                                                              });
    }

    void CubeMap::render(const gfx::GraphicsContext* const graphicsContext,
                         interlop::CubeMapRenderResources& renderResources)
    {
        graphicsContext->setGraphicsRootSignatureAndPipeline(m_cubeMapPipelineState);

        if (renderResources.textureIndex == INVALID_INDEX_U32)
        {
            renderResources.textureIndex = m_cubeMapTexture.srvIndex;
        }

        m_cubeModel->render(graphicsContext, renderResources);
    }
} // namespace helios::scene
