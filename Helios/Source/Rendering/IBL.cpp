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

        auto& computeContext = graphicsDevice->getComputeContext();
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

        const std::array<gfx::Context*, 1u> contexts = {
            computeContext.get(),
        };

        graphicsDevice->getComputeCommandQueue()->executeContext(contexts);
        graphicsDevice->getComputeCommandQueue()->flush();

        return irradianceTexture;
    }
} // namespace helios::rendering