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
        loadScene();

        loadTextures();

        loadPipelineStates();

        m_postProcessingBuffer = m_graphicsDevice->createBuffer<interlop::PostProcessingBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"Post Processing Buffer",
        });

        m_postProcessingBufferData.bloomStrength = 0.237f;

        m_deferredGPass = rendering::DeferredGeometryPass(m_graphicsDevice.get(), m_windowWidth, m_windowHeight);

        m_ibl = rendering::IBL(m_graphicsDevice.get());

        m_irradianceTexture =
            m_ibl->generateIrradianceTexture(m_graphicsDevice.get(), m_scene->m_cubeMap->m_cubeMapTexture);

        m_prefilterTexture =
            m_ibl->generatePrefilterTexture(m_graphicsDevice.get(), m_scene->m_cubeMap->m_cubeMapTexture);

        m_brdfLUTTexture = m_ibl->generateBRDFLutTexture(m_graphicsDevice.get());

        m_shadowMappingPass = rendering::PCFShadowMappingPass(m_graphicsDevice.get());

        m_ssaoPass = rendering::SSAOPass(m_graphicsDevice.get(), m_windowWidth, m_windowHeight);

        m_bloomPass = rendering::BloomPass(m_graphicsDevice.get(), m_windowWidth, m_windowHeight);
    }

    void loadScene()
    {
        m_scene->addModel(m_graphicsDevice.get(),
                          scene::ModelCreationDesc{
                              .modelPath = L"Assets/Models/DamagedHelmet/glTF/DamagedHelmet.gltf",
                              .modelName = L"Damaged Helmet",
                          });

        m_scene->addModel(m_graphicsDevice.get(),
                          scene::ModelCreationDesc{
                              .modelPath = L"Assets/Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
                              .modelName = L"MetalRough spheres",
                          });

              m_scene->addModel(m_graphicsDevice.get(), scene::ModelCreationDesc{
                                                            .modelPath = L"Assets/Models/Sponza/sponza.glb",
                                                            .modelName = L"Sponza",
                                                            .scale =
                                                                {
                                                                    0.1f,
                                                                    0.1f,
                                                                    0.1f,
                                                                },
                                                        });
        
        m_scene->addLight(
            m_graphicsDevice.get(),
            scene::LightCreationDesc{.lightType = scene::LightTypes::PointLightData,
                                     .worldSpaceLightPosition = {sinf(math::XMConvertToRadians(0)) * 15.0f, 0.0f,
                                                                 cosf(math::XMConvertToRadians(0)) * 15.0f}});

        m_scene->addLight(
            m_graphicsDevice.get(),
            scene::LightCreationDesc{.lightType = scene::LightTypes::PointLightData,
                                     .worldSpaceLightPosition = {sinf(math::XMConvertToRadians(22.50f)) * 15.0f, 0.0f,
                                                                 cosf(math::XMConvertToRadians(22.50f)) * 15.0f}});

        m_scene->addLight(m_graphicsDevice.get(),
                          scene::LightCreationDesc{.lightType = scene::LightTypes::DirectionalLightData});

        m_scene->addCubeMap(m_graphicsDevice.get(),
                            scene::CubeMapCreationDesc{
                                .equirectangularTexturePath = L"Assets/Textures/Environment.hdr",
                                .name = L"Environment Cube Map",
                            });
    }

    void loadPipelineStates()
    {

        m_pipelineState = m_graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/Shading/PBR.hlsl",
            .pipelineName = L"PBR Pipeline",
        });

        m_postProcessingPipelineState = m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/PostProcessing/PostProcessing.hlsl",
                    .pixelShaderPath = L"Shaders/PostProcessing/PostProcessing.hlsl",
                },
            .rtvFormats = {DXGI_FORMAT_R10G10B10A2_UNORM},
            .rtvCount = 1u,
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
                .rtvFormats = {DXGI_FORMAT_R10G10B10A2_UNORM},
                .rtvCount = 1u,
                .depthFormat = DXGI_FORMAT_UNKNOWN,
                .pipelineName = L"Full Screen Triangle Pass Pipeline",
            });
    }

    void loadTextures()
    {
        static constexpr std::array<uint16_t, 3u> indices = {
            0u,
            1u,
            2u,
        };

        m_renderTargetIndexBuffer = m_graphicsDevice->createBuffer<uint16_t>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::IndexBuffer,
                .name = L"Render Target Index Buffer",
            },
            indices);

        m_depthTexture = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::DepthStencil,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_D32_FLOAT,
            .name = L"Depth Texture",
        });

        m_fullScreenPassDepthTexture = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::DepthStencil,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_D32_FLOAT,
            .name = L"Full Screen Pass Depth Texture",
        });

        m_offscreenRenderTarget = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .name = L"OffScreen Render Target",
        });

        m_lightAndCubeMapRenderTarget = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"Light Render Target",
        });

        m_postProcessingRenderTarget = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_R10G10B10A2_UNORM,
            .name = L"Post Processing Render Target",
        });
    }

    void update(const float deltaTime) override
    {
        m_scene->update(deltaTime, m_input, static_cast<float>(m_windowWidth) / m_windowHeight);

        m_postProcessingBuffer.update(&m_postProcessingBufferData);
    }

    void render() override
    {
        // NOTE : In order to prevent wait for idles caused by barriers, a lot of transition barriers will be set up in
        // this render function rather than the various render passes render function to batch as many barriers as
        // possible.
        m_graphicsDevice->beginFrame();

        std::unique_ptr<gfx::GraphicsContext>& gctx = m_graphicsDevice->getCurrentGraphicsContext();
        gfx::Texture& currentBackBuffer = m_graphicsDevice->getCurrentBackBuffer();

        // const std::array<float, 4> clearColor = {std::abs(std::cosf(m_frameCount / 120.0f)), 0.0f,
        //                                          std::abs(std::sinf(m_frameCount / 120.0f)), 1.0f};
        static std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

        gctx->clearRenderTargetView(m_offscreenRenderTarget, clearColor);
        gctx->clearRenderTargetView(m_postProcessingRenderTarget, clearColor);

        gctx->clearDepthStencilView(m_depthTexture);
        gctx->clearDepthStencilView(m_fullScreenPassDepthTexture);

        // Setup resource barriers for all render passes here to lower the number of wait for idles occuring due to
        // executing resource barriers.
        gctx->addResourceBarrier(m_deferredGPass->m_gBuffer.albedoEmissiveRT.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        gctx->addResourceBarrier(m_deferredGPass->m_gBuffer.aoMetalRoughnessEmissiveRT.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        gctx->addResourceBarrier(m_deferredGPass->m_gBuffer.normalEmissiveRT.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        gctx->addResourceBarrier(m_shadowMappingPass->m_shadowDepthBuffer.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        gctx->addResourceBarrier(m_ssaoPass->m_blurSSAOTexture.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        gctx->addResourceBarrier(m_ssaoPass->m_ssaoTexture.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        gctx->addResourceBarrier(m_bloomPass->m_extractionTexture.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        gctx->addResourceBarrier(m_lightAndCubeMapRenderTarget.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        gctx->executeResourceBarriers();

        gctx->clearRenderTargetView(m_lightAndCubeMapRenderTarget, clearColor);

        gctx->setGraphicsRootSignature();

        // RenderPass 0 : Deferred GPass.
        {
            m_deferredGPass->render(m_scene.value(), gctx.get(), m_depthTexture, m_windowWidth, m_windowHeight);
        }

        // RenderPass 1 : Render lights + skybox.
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
            gctx->setRenderTarget(m_lightAndCubeMapRenderTarget, m_depthTexture);

            m_scene->renderLights(gctx.get());

            m_scene->renderCubeMap(gctx.get());
        }

        // RenderPass 2 : Shadow mapping pass.
        {
            m_shadowMappingPass->render(m_scene.value(), gctx.get());
        }

        // RenderPass 3 : SSAO Pass.
        {
            gctx->addResourceBarrier(m_depthTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            gctx->addResourceBarrier(m_deferredGPass->m_gBuffer.normalEmissiveRT.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            gctx->executeResourceBarriers();

            interlop::SSAORenderResources renderResources = {
                .normalTextureIndex = m_deferredGPass->m_gBuffer.normalEmissiveRT.srvIndex,
                .depthTextureIndex = m_depthTexture.srvIndex,
                .sceneBufferIndex = m_scene->m_sceneBuffer.cbvIndex,
            };

            gctx->setComputeRootSignature();

            m_ssaoPass->render(gctx.get(), renderResources, m_windowWidth, m_windowHeight);
        }

        // Transition all resources that are required for the shading pass but not in the appropriate resource state.
        gctx->addResourceBarrier(m_ssaoPass->m_blurSSAOTexture.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        gctx->addResourceBarrier(m_shadowMappingPass->m_shadowDepthBuffer.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        gctx->addResourceBarrier(m_deferredGPass->m_gBuffer.albedoEmissiveRT.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        gctx->addResourceBarrier(m_deferredGPass->m_gBuffer.aoMetalRoughnessEmissiveRT.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        gctx->addResourceBarrier(m_offscreenRenderTarget.allocation.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        gctx->executeResourceBarriers();

        // RenderPass 4 : Shading Pass
        {
            gctx->setComputePipelineState(m_pipelineState);

            interlop::PBRRenderResources renderResources = {
                .sceneBufferIndex = m_scene->m_sceneBuffer.cbvIndex,
                .lightBufferIndex = m_scene->m_lights->m_lightsBuffer.cbvIndex,
                .albedoEmissiveGBufferIndex = m_deferredGPass->m_gBuffer.albedoEmissiveRT.srvIndex,
                .normalEmissiveGBufferIndex = m_deferredGPass->m_gBuffer.normalEmissiveRT.srvIndex,
                .aoMetalRoughnessEmissiveGBufferIndex = m_deferredGPass->m_gBuffer.aoMetalRoughnessEmissiveRT.srvIndex,
                .irradianceTextureIndex = m_irradianceTexture.srvIndex,
                .prefilterTextureIndex = m_prefilterTexture.srvIndex,
                .brdfLUTTextureIndex = m_brdfLUTTexture.srvIndex,
                .shadowBufferIndex = m_shadowMappingPass->m_shadowBuffer.cbvIndex,
                .shadowDepthTextureIndex = m_shadowMappingPass->m_shadowDepthBuffer.srvIndex,
                .blurredSSAOTextureIndex = m_ssaoPass->m_blurSSAOTexture.srvIndex,
                .depthTextureIndex = m_depthTexture.srvIndex,
                .outputTextureIndex = m_offscreenRenderTarget.uavIndex,
            };

            gctx->set32BitComputeConstants(&renderResources);

            gctx->dispatch(std::max(m_windowWidth / 12u, 1u), std::max(m_windowHeight / 8u, 1u), 1u);
        }

        gctx->addResourceBarrier(m_depthTexture.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        gctx->addResourceBarrier(m_offscreenRenderTarget.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        gctx->addResourceBarrier(m_lightAndCubeMapRenderTarget.allocation.resource.Get(),
                                 D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        gctx->executeResourceBarriers();

        // RenderPass 5 : Bloom Pass
        {
            m_bloomPass->render(gctx.get(), m_offscreenRenderTarget, m_lightAndCubeMapRenderTarget, m_windowWidth,
                                m_windowHeight);
        }

        // RenderPass 6 : Post Processing Stage:
        {
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
            gctx->setRenderTarget(m_postProcessingRenderTarget, m_fullScreenPassDepthTexture);

            interlop::PostProcessingRenderResources renderResources = {
                .postProcessBufferIndex = m_postProcessingBuffer.cbvIndex,
                .renderTextureIndex = m_offscreenRenderTarget.srvIndex,
                .lightRenderTextureIndex = m_lightAndCubeMapRenderTarget.srvIndex,
                .ssaoTextureIndex = m_ssaoPass->m_blurSSAOTexture.srvIndex,
                .bloomTextureIndex = m_bloomPass->m_bloomUpSampleTexture.srvIndex,
            };

            gctx->set32BitGraphicsConstants(&renderResources);
            gctx->setIndexBuffer(m_renderTargetIndexBuffer);
            gctx->drawInstanceIndexed(3u);
        }

        // Render pass 7 : Render post processing render target to swapchain backbuffer via a full screen triangle pass.
        {
            gctx->addResourceBarrier(m_offscreenRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

            gctx->addResourceBarrier(m_postProcessingRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            gctx->addResourceBarrier(currentBackBuffer.allocation.resource.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                     D3D12_RESOURCE_STATE_RENDER_TARGET);

            gctx->executeResourceBarriers();

            gctx->clearRenderTargetView(currentBackBuffer, clearColor);

            gctx->setGraphicsPipelineState(m_fullScreenTrianglePassPipelineState);
            gctx->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(m_windowWidth),
                .Height = static_cast<float>(m_windowHeight),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            gctx->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gctx->setRenderTarget(currentBackBuffer);

            interlop::FullScreenTrianglePassRenderResources renderResources = {
                .renderTextureIndex = m_postProcessingRenderTarget.srvIndex,
            };

            gctx->set32BitGraphicsConstants(&renderResources);
            gctx->setIndexBuffer(m_renderTargetIndexBuffer);
            gctx->drawInstanceIndexed(3u);

            m_editor->render(m_graphicsDevice.get(), gctx.get(), m_scene.value(), m_deferredGPass->m_gBuffer,
                             m_shadowMappingPass.value(), m_ssaoPass.value(), m_bloomPass.value(),
                             m_postProcessingBufferData, m_postProcessingRenderTarget);

            gctx->addResourceBarrier(m_postProcessingRenderTarget.allocation.resource.Get(),
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

            gctx->addResourceBarrier(currentBackBuffer.allocation.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                     D3D12_RESOURCE_STATE_PRESENT);

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
    gfx::Texture m_lightAndCubeMapRenderTarget{};

    gfx::PipelineState m_pipelineState{};
    gfx::PipelineState m_postProcessingPipelineState{};
    gfx::PipelineState m_fullScreenTrianglePassPipelineState{};

    gfx::Texture m_depthTexture{};
    gfx::Texture m_fullScreenPassDepthTexture{};

    gfx::Texture m_lightTexture{};
    gfx::Texture m_lightsDepthTexture{};

    gfx::Buffer m_renderTargetIndexBuffer{};

    gfx::Buffer m_postProcessingBuffer{};
    interlop::PostProcessingBuffer m_postProcessingBufferData{};

    std::optional<rendering::DeferredGeometryPass> m_deferredGPass{};
    std::optional<rendering::IBL> m_ibl{};
    std::optional<rendering::PCFShadowMappingPass> m_shadowMappingPass{};
    std::optional<rendering::SSAOPass> m_ssaoPass{};
    std::optional<rendering::BloomPass> m_bloomPass{};

    gfx::Texture m_irradianceTexture{};
    gfx::Texture m_prefilterTexture{};
    gfx::Texture m_brdfLUTTexture{};

    uint64_t m_frameCount{};
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}