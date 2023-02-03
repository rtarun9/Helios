#include "Core/Application.hpp"

using namespace helios;
using namespace helios::core;

class SandBox final : public helios::core::Application
{
  public:
    explicit SandBox(const std::string_view windowTitle) : Application(windowTitle)
    {
    }

    void loadContent() override
    {
        static constexpr std::array<math::XMFLOAT3, 3u> triangleVertices = {
            math::XMFLOAT3{-0.5f, -0.5f, 0.0f},
            math::XMFLOAT3{0.0f, 0.5f, 0.0f},
            math::XMFLOAT3{0.5f, -0.5f, 0.0f},
        };

        m_trianglePositionBuffer = m_graphicsDevice->createBuffer<math::XMFLOAT3>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::StructuredBuffer,
                .name = L"Triangle Vertex Buffer",
            },
            triangleVertices);

        static constexpr std::array<uint32_t, 3u> triangleIndices = {0u, 1u, 2u};

        m_triangleIndexBuffer = m_graphicsDevice->createBuffer<uint32_t>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::IndexBuffer,
                .name = L"Triangle Index Buffer",
            },
            triangleIndices);

        m_trianglePipelineState = m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShader = core::ResourceManager::compileShader(
                        gfx::ShaderTypes::Vertex, getAssetsPath(L"Shaders/Triangle.hlsl"), L"VsMain"),
                    .pixelShader = ResourceManager::compileShader(gfx::ShaderTypes::Pixel,
                                                                  getAssetsPath(L"Shaders/Triangle.hlsl"), L"PsMain"),
                },
            .depthFormat = DXGI_FORMAT_UNKNOWN,
        });
    }

    void update(const float deltaTime) override
    {
    }

    void render() override
    {
        m_graphicsDevice->beginFrame();

        auto& graphicsContext = m_graphicsDevice->getCurrentGraphicsContext();

        gfx::BackBuffer& currentBackBuffer = m_graphicsDevice->getCurrentBackBuffer();

        // Prepare back buffer for rendering into it (i.e using it as a render target).
        graphicsContext->addResourceBarrier(currentBackBuffer.backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        graphicsContext->executeResourceBarriers();

        const std::array<float, 4> clearColor = {std::abs(std::cosf(m_frameCount / 120.0f)), 0.0f,
                                                 std::abs(std::sinf(m_frameCount / 120.0f)), 1.0f};

        graphicsContext->clearRenderTargetView(currentBackBuffer, clearColor);

        // Prepare for rendering.
        graphicsContext->setRenderTarget(currentBackBuffer);
        graphicsContext->setGraphicsRootSignatureAndPipeline(m_trianglePipelineState);
        graphicsContext->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const D3D12_VIEWPORT viewport = {
            .TopLeftX = 0.0,
            .TopLeftY = 0.0,
            .Width = static_cast<float>(m_windowWidth),
            .Height = static_cast<float>(m_windowHeight),
            .MinDepth = 0.0,
            .MaxDepth = 1.0,
        };
        graphicsContext->setViewport(viewport);
        graphicsContext->setIndexBuffer(m_triangleIndexBuffer);

        struct TriangleRenderResources
        {
            uint32_t positionBufferIndex;
        };

        const TriangleRenderResources triangleRenderResources = {
            .positionBufferIndex = m_trianglePositionBuffer.srvIndex,
        };

        graphicsContext->set32BitGraphicsConstants(&triangleRenderResources);

        graphicsContext->drawInstanceIndexed(3u, 1u);

        // Prepare back buffer for presentation.
        graphicsContext->addResourceBarrier(currentBackBuffer.backBufferResource.Get(),
                                            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        graphicsContext->executeResourceBarriers();

        const std::array<gfx::Context* const, 1u> contexts = {
            graphicsContext.get(),
        };

        m_graphicsDevice->getDirectCommandQueue()->executeContext(contexts);

        m_graphicsDevice->present();
        m_graphicsDevice->endFrame();

        m_frameCount++;
    }

  private:
    uint64_t m_frameCount{};

    gfx::Buffer m_trianglePositionBuffer{};
    gfx::Buffer m_triangleIndexBuffer{};
    gfx::PipelineState m_trianglePipelineState{};
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}