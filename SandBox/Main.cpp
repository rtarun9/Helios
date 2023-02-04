#include "Core/Application.hpp"

using namespace helios;

class SandBox final : public helios::core::Application
{
  public:
    explicit SandBox(const std::string_view windowTitle) : Application(windowTitle)
    {
    }

    void loadContent() override
    {
        static constexpr std::array<math::XMFLOAT3, 3> trianglePositionData = {
            math::XMFLOAT3{-0.5f, -0.5f, 0.0f},
            math::XMFLOAT3{0.0f, 0.5f, 0.0f},
            math::XMFLOAT3{0.5f, -0.5f, 0.0f},
        };

        m_trianglePositionBuffer = m_graphicsDevice->createBuffer<math::XMFLOAT3>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::StructuredBuffer,
                .name = L"Triangle Position Buffer",
            },
            trianglePositionData);

        static constexpr std::array<math::XMFLOAT2, 3> triangleTextureCoordData = {
            math::XMFLOAT2{1.0f, 0.0f},
            math::XMFLOAT2{0.5f, 0.0f},
            math::XMFLOAT2{0.0f, 1.0f},
        };

        m_triangleTextureCoordBuffer = m_graphicsDevice->createBuffer<math::XMFLOAT2>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::StructuredBuffer,
                .name = L"Triangle Color Buffer",
            },
            triangleTextureCoordData);

        static constexpr std::array<uint32_t, 3> triangleIndicesData = {
            0u,
            1u,
            2u,
        };

        m_triangleIndexBuffer = m_graphicsDevice->createBuffer<uint32_t>(
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::IndexBuffer,
                .name = L"Triangle Index Buffer",
            },
            triangleIndicesData);

        m_texture = m_graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::TextureFromPath,
            .name = L"Test Texture",
            .path = L"Assets/Models/DamagedHelmet/glTF/Default_AO.jpg",
        });

        m_trianglePipelineState = m_graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/Triangle.hlsl",
                    .pixelShaderPath = L"Shaders/Triangle.hlsl",
                },
            .depthFormat = DXGI_FORMAT_UNKNOWN,
            .pipelineName = L"Triangle Pipeline",
        });
    }

    void update(const float deltaTime) override
    {
    }

    void render() override
    {
        m_graphicsDevice->beginFrame();

        std::unique_ptr<gfx::GraphicsContext>& gctx = m_graphicsDevice->getCurrentGraphicsContext();
        gfx::BackBuffer& currentBackBuffer = m_graphicsDevice->getCurrentBackBuffer();

        // Prepare back buffer for rendering into it (i.e using it as a render target).
        gctx->addResourceBarrier(currentBackBuffer.backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                 D3D12_RESOURCE_STATE_RENDER_TARGET);
        gctx->executeResourceBarriers();

        const std::array<float, 4> clearColor = {std::abs(std::cosf(m_frameCount / 120.0f)), 0.0f,
                                                 std::abs(std::sinf(m_frameCount / 120.0f)), 1.0f};

        gctx->clearRenderTargetView(currentBackBuffer, clearColor);

        // Prepare for rendering.

        gctx->setGraphicsRootSignatureAndPipeline(m_trianglePipelineState);
        gctx->setViewport(D3D12_VIEWPORT{
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<float>(m_windowWidth),
            .Height = static_cast<float>(m_windowHeight),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        });

        gctx->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gctx->setIndexBuffer(m_triangleIndexBuffer);
        gctx->setRenderTarget(currentBackBuffer);

        struct TriangleRenderResources
        {
            uint32_t positionBufferIndex;
            uint32_t textureCoordBufferIndex;
            uint32_t textureIndex;
        };

        const TriangleRenderResources renderResources = {
            .positionBufferIndex = m_trianglePositionBuffer.srvIndex,
            .textureCoordBufferIndex = m_triangleTextureCoordBuffer.srvIndex,
            .textureIndex = m_texture.srvIndex,
        };

        gctx->set32BitGraphicsConstants(&renderResources);

        gctx->drawInstanceIndexed(3u, 1u);

        // Prepare back buffer for presentation.
        gctx->addResourceBarrier(currentBackBuffer.backBufferResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
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
    uint64_t m_frameCount{};
    gfx::Buffer m_trianglePositionBuffer{};
    gfx::Buffer m_triangleTextureCoordBuffer{};
    gfx::Buffer m_triangleIndexBuffer{};

    gfx::Texture m_texture{};

    gfx::PipelineState m_trianglePipelineState{};
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}