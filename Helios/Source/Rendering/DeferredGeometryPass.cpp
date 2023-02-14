#include "Rendering/DeferredGeometryPass.hpp"

#include "Graphics/GraphicsDevice.hpp"

// GBuffer composition:
// float4 albedo : SV_Target0;
// float4 positionEmissive : SV_Target1;
// float4 normalEmissive : SV_Target2;
// float4 aoMetalRoughnessEmissive : SV_Target3;

namespace helios::rendering
{
    DeferredGeometryPass::DeferredGeometryPass(const gfx::GraphicsDevice* const graphicsDevice, const uint32_t width,
                                               const uint32_t height)
    {
        // Create pipeline state.
        m_deferredGPassPipelineState = graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/RenderPass/DeferredGeometryPass.hlsl",
                    .pixelShaderPath = L"Shaders/RenderPass/DeferredGeometryPass.hlsl",
                },
            .rtvFormats =
                {
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    DXGI_FORMAT_R16G16B16A16_FLOAT,
                    DXGI_FORMAT_R16G16B16A16_FLOAT,
                    DXGI_FORMAT_R16G16B16A16_FLOAT,
                },
            .rtvCount = 4,
            .pipelineName = L"Deferred Geometry Pass Pipeline",
        });

        // Create MRT's for GBuffer.
        m_gBuffer.albedoRT = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"Deferred Pass Albedo Texture",
        });

        m_gBuffer.positionEmissiveRT = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            gfx::TextureCreationDesc{
                .usage = gfx::TextureUsage::RenderTarget,
                .width = width,
                .height = height,
                .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
                .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                .name = L"Deferred Pass Position Emissive Texture",
            },
        });

        m_gBuffer.normalEmissiveRT = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"Deferred Pass Normal Emissive Texture",
        });

        m_gBuffer.aoMetalRoughnessEmissiveRT = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"Deferred Pass AO Metal Roughness Emissive Texture",
        });
    }

    void DeferredGeometryPass::render(scene::Scene* const scene, gfx::GraphicsContext* const graphicsContext,
                                      gfx::Texture& depthBuffer, const uint32_t width, const uint32_t height)
    {
        std::array<const gfx::Texture, 4u> renderTargets = {

            m_gBuffer.albedoRT,
            m_gBuffer.positionEmissiveRT,
            m_gBuffer.normalEmissiveRT,
            m_gBuffer.aoMetalRoughnessEmissiveRT,
        };

        for (const auto& renderTarget : renderTargets)
        {
            graphicsContext->addResourceBarrier(renderTarget.allocation.resource.Get(),
                                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        graphicsContext->executeResourceBarriers();

        graphicsContext->setGraphicsRootSignatureAndPipeline(m_deferredGPassPipelineState);
        graphicsContext->setRenderTarget(renderTargets, depthBuffer);
        graphicsContext->setViewport(D3D12_VIEWPORT{
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<float>(width),
            .Height = static_cast<float>(height),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        });

        graphicsContext->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        graphicsContext->clearRenderTargetView(renderTargets, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 0.0f});
        graphicsContext->clearDepthStencilView(depthBuffer);

        interlop::DeferredGPassRenderResources deferredGPassRenderResources{};

        scene->renderModels(graphicsContext, deferredGPassRenderResources);

        for (const auto& renderTarget : renderTargets)
        {
            graphicsContext->addResourceBarrier(renderTarget.allocation.resource.Get(),
                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        graphicsContext->executeResourceBarriers();
    }
} // namespace helios::renderpass