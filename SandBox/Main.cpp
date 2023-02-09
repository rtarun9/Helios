#include "Helios.hpp"

using namespace helios;

class SandBox final : public helios::core::Application
{
  public:
    explicit SandBox(const std::string_view windowTitle) : Application(windowTitle)
    {
    }

    void loadContent() override
    {
        m_scene->addModel(m_graphicsDevice.get(),
                          scene::ModelCreationDesc{
                              .modelPath = L"Assets/Models/DamagedHelmet/glTF/DamagedHelmet.gltf",
                              .modelName = L"Damaged Helmet",
                          });

        m_scene->addModel(m_graphicsDevice.get(),
                          scene::ModelCreationDesc{.modelPath = L"Assets/Models/Sponza/glTF/Sponza.gltf",
                                                   .modelName = L"Sponza",
                                                   .scale = {
                                                       0.1f,
                                                       0.1f,
                                                       0.1f,
                                                   }});

        m_scene->addLight(m_graphicsDevice.get(),
                          scene::LightCreationDesc{.lightType = scene::LightTypes::PointLightData});

        static constexpr std::array<uint32_t, 3u> indices = {
            0u,
            1u,
            2u,
        };

        m_renderTargetIndexBuffer = m_graphicsDevice->createBuffer<uint32_t>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::IndexBuffer,
                .name = L"Render Target Index Buffer",
            },
            indices);

        m_pipelineState = m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/Shading/BlinnPhong.hlsl",
                    .pixelShaderPath = L"Shaders/Shading/BlinnPhong.hlsl",
                },
            .depthFormat = DXGI_FORMAT_UNKNOWN,
            .pipelineName = L"Blinn Phong Pipeline",
        });

        m_depthTexture = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::DepthStencil,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_D32_FLOAT,
            .name = L"Depth Texture",
        });

        m_postProcessingDepthTexture = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::DepthStencil,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_D32_FLOAT,
            .name = L"Depth Texture",
        });

        m_offscreenRenderTarget = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_R10G10B10A2_UNORM,
            .name = L"OffScreen Render Target",
        });

        m_postProcessingRenderTarget = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_R10G10B10A2_UNORM,
            .name = L"Post Processing Render Target",
        });

        m_finalRenderTarget = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_R10G10B10A2_UNORM,
            .name = L"Final Target",
        });

        m_postProcessingPipelineState = m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/PostProcessing/PostProcessing.hlsl",
                    .pixelShaderPath = L"Shaders/PostProcessing/PostProcessing.hlsl",
                },
            .depthFormat = DXGI_FORMAT_D32_FLOAT,
            .pipelineName = L"Post Processing Pipeline",
        });

        m_fullScreenTrianglePassPipelineState =
            m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
                .shaderModule =
                    {
                        .vertexShaderPath = L"Shaders/RenderPass/FullScreenTrianglePass.hlsl",
                        .pixelShaderPath = L"Shaders/RenderPass/FullScreenTrianglePass.hlsl",
                    },
                .depthFormat = DXGI_FORMAT_UNKNOWN,
                .pipelineName = L"Full Screen Triangle Pass Pipeline",
            });

        m_deferredGPass =
            std::make_unique<renderpass::DeferredGeometryPass>(m_graphicsDevice.get(), m_windowWidth, m_windowHeight);
    }

    void update(const float deltaTime) override
    {
        m_scene->update(deltaTime, m_input, static_cast<float>(m_windowWidth) / m_windowHeight);
    }

    void render() override
    {
        m_graphicsDevice->beginFrame();

        std::unique_ptr<gfx::GraphicsContext>& gctx = m_graphicsDevice->getCurrentGraphicsContext();
        gfx::Texture& currentBackBuffer = m_graphicsDevice->getCurrentBackBuffer();

        // const std::array<float, 4> clearColor = {std::abs(std::cosf(m_frameCount / 120.0f)), 0.0f,
        //                                          std::abs(std::sinf(m_frameCount / 120.0f)), 1.0f};
        static std::array<float, 4> clearColor = {0.2f, 0.2f, 0.2f, 1.0f};

        gctx->clearRenderTargetView(m_offscreenRenderTarget, clearColor);
        gctx->clearRenderTargetView(m_postProcessingRenderTarget, clearColor);
        gctx->clearRenderTargetView(m_finalRenderTarget, clearColor);

        gctx->clearDepthStencilView(m_depthTexture);
        gctx->clearDepthStencilView(m_postProcessingDepthTexture);

        // RenderPass 0 : Deferred GPass.
        {
            m_deferredGPass->render(m_scene.get(), gctx.get(), m_depthTexture, m_windowWidth, m_windowHeight);
        }

        // RenderPass 1 : Lighting Pass
        {
            gctx->setGraphicsRootSignatureAndPipeline(m_pipelineState);
            gctx->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(m_windowWidth),
                .Height = static_cast<float>(m_windowHeight),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            gctx->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gctx->setRenderTarget(m_offscreenRenderTarget);

            interlop::BlinnPhongRenderResources renderResources = {
                .albedoGBufferIndex = m_deferredGPass->m_gBuffer.albedoRT.srvIndex,
                .positionEmissiveGBufferIndex = m_deferredGPass->m_gBuffer.positionEmissiveRT.srvIndex,
                .normalEmissiveGBufferIndex = m_deferredGPass->m_gBuffer.normalEmissiveRT.srvIndex,
                .aoMetalRoughnessEmissiveGBufferIndex = m_deferredGPass->m_gBuffer.aoMetalRoughnessEmissiveRT.srvIndex,
            };

            m_scene->renderModels(gctx.get(), renderResources);
        }

        // RenderPass 2 : Render lights using forward rendering.
        {
            gctx->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(m_windowWidth),
                .Height = static_cast<float>(m_windowHeight),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            gctx->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gctx->setRenderTarget(m_offscreenRenderTarget, m_depthTexture);

            m_scene->renderLights(gctx.get());
        }

        // RenderPass 3 : Post Processing Stage:
        {
            gctx->addResourceBarrier(m_offscreenRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            gctx->executeResourceBarriers();

            gctx->setGraphicsRootSignatureAndPipeline(m_postProcessingPipelineState);
            gctx->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(m_windowWidth),
                .Height = static_cast<float>(m_windowHeight),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            gctx->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gctx->setRenderTarget(m_postProcessingRenderTarget, m_postProcessingDepthTexture);

            interlop::PostProcessingRenderResources renderResources = {
                .renderTextureIndex = m_offscreenRenderTarget.srvIndex,
            };

            gctx->set32BitGraphicsConstants(&renderResources);
            gctx->setIndexBuffer(m_renderTargetIndexBuffer);
            gctx->drawInstanceIndexed(3u);
        }

        // Render pass 2 : Prepare the final render target. The contents of this RT is copied into the back buffer.
        {
            gctx->addResourceBarrier(m_offscreenRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

            gctx->addResourceBarrier(m_postProcessingRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            gctx->executeResourceBarriers();

            gctx->setGraphicsRootSignatureAndPipeline(m_fullScreenTrianglePassPipelineState);
            gctx->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(m_windowWidth),
                .Height = static_cast<float>(m_windowHeight),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            gctx->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gctx->setRenderTarget(m_finalRenderTarget);

            interlop::FullScreenTrianglePassRenderResources renderResources = {
                .renderTextureIndex = m_postProcessingRenderTarget.srvIndex,
            };

            gctx->set32BitGraphicsConstants(&renderResources);
            gctx->setIndexBuffer(m_renderTargetIndexBuffer);
            gctx->drawInstanceIndexed(3u);

            m_editor->render(m_graphicsDevice.get(), m_scene.get(), clearColor, &m_postProcessingRenderTarget,
                             gctx.get());
        }

        // RenderPass 4 : Copy the final render target into the back buffer and present.
        {
            gctx->addResourceBarrier(m_postProcessingRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

            gctx->addResourceBarrier(m_finalRenderTarget.allocation.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                     D3D12_RESOURCE_STATE_COPY_SOURCE);

            gctx->addResourceBarrier(currentBackBuffer.allocation.resource.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                     D3D12_RESOURCE_STATE_COPY_DEST);
            gctx->executeResourceBarriers();

            gctx->copyResource(m_finalRenderTarget.allocation.resource.Get(),
                               currentBackBuffer.allocation.resource.Get());

            // Prepare back buffer for presentation.
            gctx->addResourceBarrier(currentBackBuffer.allocation.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                     D3D12_RESOURCE_STATE_PRESENT);

            gctx->addResourceBarrier(m_finalRenderTarget.allocation.resource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                     D3D12_RESOURCE_STATE_RENDER_TARGET);

            gctx->executeResourceBarriers();
        }

        const std::array<gfx::Context* const, 1u> contexts = {
            gctx.get(),
        };

        m_graphicsDevice->getDirectCommandQueue()->executeContext(contexts);

        m_graphicsDevice->present();
        m_graphicsDevice->endFrame();

        m_frameCount++;
    }

  private:
    gfx::Texture m_offscreenRenderTarget{};
    gfx::Texture m_postProcessingRenderTarget{};

    // The contents of the final render pass is copied into the swapchain back buffer.
    gfx::Texture m_finalRenderTarget{};

    gfx::PipelineState m_pipelineState{};
    gfx::PipelineState m_postProcessingPipelineState{};
    gfx::PipelineState m_fullScreenTrianglePassPipelineState{};

    gfx::Texture m_depthTexture{};
    gfx::Texture m_postProcessingDepthTexture{};

    gfx::Buffer m_renderTargetIndexBuffer{};

    std::unique_ptr<renderpass::DeferredGeometryPass> m_deferredGPass{};

    uint64_t m_frameCount{};
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}