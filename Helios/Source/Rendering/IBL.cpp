#include "Rendering/IBL.hpp"

#include "Graphics/ComputeContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "ShaderInterlop/RenderResources.hlsli"

namespace helios::rendering
{
    IBL::IBL(gfx::GraphicsDevice* const graphicsDevice)
    {
        m_irradianceConvolutionPipelineState =
            graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
                .csShaderPath = L"Shaders/IBL/DiffuseIrradianceCS.hlsl",
                .pipelineName = L"Diffuse Irradiance Pipeline State",
            });

        m_prefilterConvolutionPipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/IBL/SpecularPrefilterCS.hlsl",
            .pipelineName = L"Specular Prefilter Pipeline State",
        });

        m_brdfLUTPipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/IBL/BRDFLutCS.hlsl",
            .pipelineName = L"BRDF LUT Pipeline State",
        });
    }

    gfx::Texture IBL::generateIrradianceTexture(gfx::GraphicsDevice* const graphicsDevice,
                                                const gfx::Texture& cubeMapTexture)
    {
        // Create the irradiance texture.
        gfx::Texture irradianceTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::CubeMap,
            .width = IRRADIANCE_MAP_TEXTURE_DIMENSION,
            .height = IRRADIANCE_MAP_TEXTURE_DIMENSION,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .mipLevels = 1u,
            .depthOrArraySize = 6u,
            .name = L"Irradiance Map",
        });

        // Run the compute shader for irradiance map computation.

        auto computeContext = graphicsDevice->getComputeContext();
        computeContext->reset();
        computeContext->addResourceBarrier(irradianceTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_COMMON,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        computeContext->executeResourceBarriers();

        computeContext->setComputeRootSignatureAndPipeline(m_irradianceConvolutionPipelineState);

        interlop::IrradianceRenderResources diffuseIrradianceRenderResources{
            .skyBoxTextureIndex = cubeMapTexture.srvIndex,
            .ouputIrradianceMapIndex = irradianceTexture.uavIndex,
        };

        computeContext->set32BitComputeConstants(&diffuseIrradianceRenderResources);

        computeContext->dispatch(IRRADIANCE_MAP_TEXTURE_DIMENSION / 8u, IRRADIANCE_MAP_TEXTURE_DIMENSION / 8u, 6u);

        computeContext->addResourceBarrier(irradianceTexture.allocation.resource.Get(),
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
        computeContext->executeResourceBarriers();
        graphicsDevice->executeAndFlushComputeContext(std::move(computeContext));

        return irradianceTexture;
    }

    gfx::Texture IBL::generatePrefilterTexture(gfx::GraphicsDevice* const graphicsDevice,
                                               const gfx::Texture& cubeMapTexture)
    {
        // Create Pre Filter map texture.
        gfx::Texture prefilterTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::CubeMap,
            .width = PREFILTER_MAP_TEXTURE_DIMENSION,
            .height = PREFILTER_MAP_TEXTURE_DIMENSION,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .mipLevels = 7u,
            .depthOrArraySize = 6u,
            .name = L"Specular Pre Filter Map",
        });

        // Run compute shader to generate prefilter map from skybox texture.
        auto computeContext = graphicsDevice->getComputeContext();
        computeContext->reset();

        computeContext->addResourceBarrier(prefilterTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_COMMON,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        computeContext->executeResourceBarriers();

        computeContext->setComputeRootSignatureAndPipeline(m_prefilterConvolutionPipelineState);

        uint32_t size{PREFILTER_MAP_TEXTURE_DIMENSION};
        for (uint32_t i = 0; i < 7u; i++)
        {
            const uint32_t uavIndex = prefilterTexture.uavIndex + i;

            interlop::PreFilterRenderResources renderResources = {
                .skyBoxTextureIndex = cubeMapTexture.srvIndex,
                .outputPreFilteredCubeMapIndex = uavIndex,
                .mipLevel = i,
            };

            const uint32_t numGroups = std::max(1u, size / 8u);

            computeContext->set32BitComputeConstants(&renderResources);

            computeContext->dispatch(numGroups, numGroups, 6u);
            size /= 2;
        }

        computeContext->addResourceBarrier(prefilterTexture.allocation.resource.Get(),
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
        computeContext->executeResourceBarriers();

        graphicsDevice->executeAndFlushComputeContext(std::move(computeContext));

        return prefilterTexture;
    }

    gfx::Texture IBL::generateBRDFLutTexture(gfx::GraphicsDevice* const graphicsDevice)
    {
        gfx::Texture brdfLutTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::UAVTexture,
            .width = BRDF_LUT_TEXTURE_DIMENSION,
            .height = BRDF_LUT_TEXTURE_DIMENSION,
            .format = DXGI_FORMAT_R16G16_FLOAT,
            .mipLevels = 1u,
            .depthOrArraySize = 1u,
            .name = L"BRDF LUT Texture Map",
        });

        // Run compute shader to generate BRDF LUT.
        auto computeContext = graphicsDevice->getComputeContext();
        computeContext->reset();

        computeContext->addResourceBarrier(brdfLutTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_COMMON,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        computeContext->executeResourceBarriers();

        computeContext->setComputeRootSignatureAndPipeline(m_brdfLUTPipelineState);

        interlop::BRDFLutRenderResources brdfLutRenderResources = {
            .lutTextureIndex = brdfLutTexture.uavIndex,
        };

        computeContext->set32BitComputeConstants(&brdfLutRenderResources);

        computeContext->dispatch(BRDF_LUT_TEXTURE_DIMENSION / 32u, BRDF_LUT_TEXTURE_DIMENSION / 32u, 1u);

        computeContext->addResourceBarrier(brdfLutTexture.allocation.resource.Get(),
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
        computeContext->executeResourceBarriers();

        graphicsDevice->executeAndFlushComputeContext(std::move(computeContext));

        return brdfLutTexture;
    }
} // namespace helios::rendering