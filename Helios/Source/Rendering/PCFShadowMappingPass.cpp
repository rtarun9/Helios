#include "Rendering/PCFShadowMappingPass.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "Scene/Scene.hpp"

using namespace math;

namespace helios::rendering
{
    PCFShadowMappingPass::PCFShadowMappingPass(gfx::GraphicsDevice* const graphicsDevice)
    {
        m_shadowPassPipelineState = graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/RenderPass/PCFShadowPass.hlsl",
                    .pixelShaderPath = L"Shaders/RenderPass/PCFShadowPass.hlsl",
                },
            .rtvCount = 0u,
            .cullMode = D3D12_CULL_MODE_FRONT,
            .pipelineName = L"PCF Shadow Pass Pipeline State",
        });

        m_shadowDepthBuffer = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::DepthStencil,
            .width = SHADOW_MAP_DIMENSIONS,
            .height = SHADOW_MAP_DIMENSIONS,
            .format = DXGI_FORMAT_D32_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"PCF Shadow Mapping Pass Depth Texture",
        });

        m_shadowBuffer = graphicsDevice->createBuffer<interlop::SceneBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"Shadow Buffer",
        });

        m_shadowBufferData = {
            .backOffDistance = 200.0f,
            .extents = 180.0f,
            .nearPlane = 1.0f,
            .farPlane = 370.0f,
        };

        m_shadowBuffer.update(&m_shadowBufferData);
    }

    void PCFShadowMappingPass::render(scene::Scene& scene, gfx::GraphicsContext* const graphicsContext)
    {
        const auto directionalLightPosition = scene.m_lights->m_lightsBufferData.lightPosition[0];
        const auto directionalLightPositionVector = math::XMLoadFloat4(&directionalLightPosition);

        const math::XMVECTOR lightPosition =
            math::XMVectorZero() -
            m_shadowBufferData.backOffDistance * math::XMVector4Normalize(directionalLightPositionVector);

        const math::XMMATRIX lightViewMatrix = math::XMMatrixLookAtLH(lightPosition, DirectX::XMVectorZero(),
                                                                         DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        const math::XMMATRIX lightProjectionMatrix = math::XMMatrixOrthographicOffCenterLH(
            -m_shadowBufferData.extents, m_shadowBufferData.extents, -m_shadowBufferData.extents,
            m_shadowBufferData.extents, m_shadowBufferData.nearPlane, m_shadowBufferData.farPlane);

        m_shadowBufferData.lightViewProjectionMatrix = lightViewMatrix * lightProjectionMatrix;
        m_shadowBuffer.update(&m_shadowBufferData);

        graphicsContext->setViewport(D3D12_VIEWPORT{
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<float>(SHADOW_MAP_DIMENSIONS),
            .Height = static_cast<float>(SHADOW_MAP_DIMENSIONS),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        });

        const std::array<gfx::Texture, 0u> nullRtvTextures = {};
        
        // This resource barrier can be batched (and done in the sandbox main code)
        // graphicsContext->addResourceBarrier(m_shadowDepthBuffer.allocation.resource.Get(),
        //                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        //                                     D3D12_RESOURCE_STATE_DEPTH_WRITE);
        // graphicsContext->executeResourceBarriers();

        graphicsContext->setGraphicsRootSignatureAndPipeline(m_shadowPassPipelineState);
        graphicsContext->clearDepthStencilView(m_shadowDepthBuffer);
        graphicsContext->setRenderTarget(nullRtvTextures, m_shadowDepthBuffer);

        graphicsContext->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


        interlop::ShadowPassRenderResources shadowRenderResources{
            .shadowBufferIndex = m_shadowBuffer.cbvIndex,
        };

        scene.renderModels(graphicsContext, shadowRenderResources);

        // This transition is not required till the shading passes, hence moved to the SandBox's render loop.
        // graphicsContext->addResourceBarrier(m_shadowDepthBuffer.allocation.resource.Get(),
        //                                     D3D12_RESOURCE_STATE_DEPTH_WRITE,
        //                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        // graphicsContext->executeResourceBarriers();
    }
} // namespace helios::rendering