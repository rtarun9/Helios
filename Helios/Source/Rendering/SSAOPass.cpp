#include "Rendering/SSAOPass.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "ShaderInterlop/RenderResources.hlsli"

using namespace math;

namespace helios::rendering
{
    SSAOPass::SSAOPass(gfx::GraphicsDevice* const graphicsDevice, const uint32_t width, const uint32_t height)
    {
        // Generate the sample kernel, that is points in a hemisphere with z in the range [0, 1].
        // The hemisphere must be a unit hemisphere.
        // Sample points must be more clustered towards origin.

        std::uniform_real_distribution<float> unitDistribution{0.0f, 1.0f};
        std::uniform_real_distribution<float> minusOneToOneDistribution{-1.0f, 1.0f};
        std::default_random_engine generator{};

        m_ssaoBufferData.sampleVectorCount = 128;

        for (const uint32_t i : std::views::iota(0u, m_ssaoBufferData.sampleVectorCount))
        {
            math::XMVECTOR samplePosition = math::XMVector3Normalize(
                math::XMVectorSet(minusOneToOneDistribution(generator), minusOneToOneDistribution(generator),
                                  unitDistribution(generator), 0.0f));

            // Distribute these vectors over the hemisphere.
            samplePosition = samplePosition * unitDistribution(generator);

            // Sample points nearer to the point have more weightage and will occlude points more than if they were
            // farther away. The approach we take to get such sample points, is to have a variable scale value.
            // Normalizing i into a range of 0 -> 1, we compute (normalized i)^2. If this value is low, make the sample
            // position close to the point (i.e the scale will be low). As (normalized i) gets larger in a quadratic
            // fashion, the scale goes more towards 1. Since normalized i is between 0 and 1, the square grows slowly.
            // This means more sample positions will be close to the origin than farther away.

            const float normalizedI = static_cast<float>(i) / 64.0f;
            const float scale = std::lerp<float, float>(0.1f, 1.0f, normalizedI * normalizedI);

            samplePosition = samplePosition * scale;
            math::XMStoreFloat4(&m_ssaoBufferData.sampleVectors[i], samplePosition);
        }

        m_ssaoBuffer = graphicsDevice->createBuffer<interlop::SSAOBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"SSAO Buffer",
        });

        m_ssaoBufferData.bias = 0.022f;
        m_ssaoBufferData.radius = 5.0f;

        m_ssaoBuffer.update(&m_ssaoBufferData);

        // Create the random rotation texture (a 4/4 texture in range ([-1, 1], [-1, 1]).
        m_ssaoBufferData.noiseTextureWidth = 4.0f;
        m_ssaoBufferData.noiseTextureHeight = 4.0f;

        std::array<math::XMFLOAT2, 16> randomRotationTextureData{};
        for (const uint32_t i : std::views::iota(0u, 16u))
        {
            randomRotationTextureData[i] =
                math::XMFLOAT2(minusOneToOneDistribution(generator), minusOneToOneDistribution(generator));
        }

        m_randomRotationTexture = graphicsDevice->createTexture(
            gfx::TextureCreationDesc{
                .usage = gfx::TextureUsage::TextureFromData,
                .width = 4u,
                .height = 4u,
                .format = DXGI_FORMAT_R8G8_SNORM,
                .bytesPerPixel = 2u,
                .name = L"Random Rotation Texture",
            },
            randomRotationTextureData.data());

        // Create render targets and pipeline states.
        m_ssaoTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R32_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"SSAO Texture Texture",
        });

        m_ssaoPipelineState = graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/RenderPass/SSAOPass.hlsl",
                    .pixelShaderPath = L"Shaders/RenderPass/SSAOPass.hlsl",
                },
            .rtvFormats = {DXGI_FORMAT_R32_FLOAT},
            .depthFormat = DXGI_FORMAT_UNKNOWN,
            .pipelineName = L"SSAO Pipeline State",
        });

        m_blurSSAOTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R32_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .name = L"SSAO Texture Texture",
        });

        m_boxBlurPipelineState = graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/PostProcessing/BoxBlur.hlsl",
                    .pixelShaderPath = L"Shaders/PostProcessing/BoxBlur.hlsl",
                },
            .rtvFormats = {DXGI_FORMAT_R32_FLOAT},
            .depthFormat = DXGI_FORMAT_UNKNOWN,
            .pipelineName = L"Box Blur Pipeline State",
        });
    }

    void SSAOPass::render(gfx::GraphicsContext* const graphicsContext, const gfx::Buffer& renderTargetIndexBuffer,
                          interlop::SSAORenderResources& renderResources, const uint32_t width, const uint32_t height)
    {
        m_ssaoBufferData.screenWidth = width;
        m_ssaoBufferData.screenHeight = height;

        // Put this in update maybe?
        m_ssaoBuffer.update(&m_ssaoBufferData);

        graphicsContext->addResourceBarrier(m_ssaoTexture.allocation.resource.Get(),
                                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);

        graphicsContext->addResourceBarrier(m_blurSSAOTexture.allocation.resource.Get(),
                                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);

        graphicsContext->executeResourceBarriers();

        // Create the SSAO texture.
        {
            graphicsContext->setGraphicsRootSignatureAndPipeline(m_ssaoPipelineState);
            graphicsContext->setRenderTarget(m_ssaoTexture);
            graphicsContext->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(width),
                .Height = static_cast<float>(height),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            graphicsContext->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            graphicsContext->clearRenderTargetView(m_ssaoTexture, 1.0f);

            renderResources.randomRotationTextureIndex = m_randomRotationTexture.srvIndex;
            renderResources.ssaoBufferIndex = m_ssaoBuffer.cbvIndex;

            graphicsContext->set32BitGraphicsConstants(&renderResources);

            graphicsContext->setIndexBuffer(renderTargetIndexBuffer);
            graphicsContext->drawIndexed(3u);

            graphicsContext->addResourceBarrier(m_ssaoTexture.allocation.resource.Get(),
                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            graphicsContext->executeResourceBarriers();
        }

        // Blur the ssao texture.
        {
            graphicsContext->setGraphicsRootSignatureAndPipeline(m_boxBlurPipelineState);
            graphicsContext->setRenderTarget(m_blurSSAOTexture);
            graphicsContext->setViewport(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = static_cast<float>(width),
                .Height = static_cast<float>(height),
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });

            graphicsContext->setPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            graphicsContext->clearRenderTargetView(m_blurSSAOTexture, 1.0f);

            const interlop::BoxBlurRenderResources blurRenderResources = {
                .textureIndex = m_ssaoTexture.srvIndex,
            };

            graphicsContext->set32BitGraphicsConstants(&blurRenderResources);

            graphicsContext->setIndexBuffer(renderTargetIndexBuffer);
            graphicsContext->drawIndexed(3u);
        }

        // Transition blur texture back to pixel shader resource.

        graphicsContext->addResourceBarrier(m_blurSSAOTexture.allocation.resource.Get(),
                                            D3D12_RESOURCE_STATE_RENDER_TARGET,
                                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        graphicsContext->executeResourceBarriers();
    }
} // namespace helios::rendering
