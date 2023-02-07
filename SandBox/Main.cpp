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

        m_pipelineState = m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/ModelViewer.hlsl",
                    .pixelShaderPath = L"Shaders/ModelViewer.hlsl",
                },
            .depthFormat = DXGI_FORMAT_D32_FLOAT,
            .pipelineName = L"ModelViewer Pipeline",
        });

        m_depthTexture = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::DepthStencil,
            .width = m_windowWidth,
            .height = m_windowHeight,
            .format = DXGI_FORMAT_D32_FLOAT,
            .name = L"Depth Texture",
        });
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

        // Prepare back buffer for rendering into it (i.e using it as a render target).
        gctx->addResourceBarrier(currentBackBuffer.allocation.resource.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                 D3D12_RESOURCE_STATE_RENDER_TARGET);
        gctx->executeResourceBarriers();

        // const std::array<float, 4> clearColor = {std::abs(std::cosf(m_frameCount / 120.0f)), 0.0f,
        //                                          std::abs(std::sinf(m_frameCount / 120.0f)), 1.0f};
        std::array<float, 4> clearColor = {0.2f, 0.2f, 0.2f, 1.0f};

        gctx->clearRenderTargetView(currentBackBuffer, clearColor);
        gctx->clearDepthStencilView(m_depthTexture);

        // Prepare for rendering.

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
        gctx->setRenderTarget(currentBackBuffer, m_depthTexture);

        m_scene->renderModels(gctx.get());

        m_editor->render(m_graphicsDevice.get(), m_scene.get(), clearColor, &currentBackBuffer, gctx.get());

        // Prepare back buffer for presentation.
        gctx->addResourceBarrier(currentBackBuffer.allocation.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                 D3D12_RESOURCE_STATE_PRESENT);

        gctx->executeResourceBarriers();

        const std::array<gfx::Context* const, 1u> contexts = {
            gctx.get(),
        };

        m_graphicsDevice->getDirectCommandQueue()->executeContext(contexts);

        m_graphicsDevice->present();
        m_graphicsDevice->endFrame();

        m_frameCount++;
    }

  private:
    gfx::Texture m_texture{};

    gfx::PipelineState m_pipelineState{};

    gfx::Texture m_depthTexture{};

    uint64_t m_frameCount{};
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}