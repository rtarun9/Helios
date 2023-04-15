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

        for (const uint32_t i : std::views::iota(0u, interlop::SAMPLE_VECTOR_COUNT))
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

            const float normalizedI = static_cast<float>(i) / static_cast<float>(interlop::SAMPLE_VECTOR_COUNT);
            const float scale = std::lerp<float, float>(0.1f, 1.0f, normalizedI * normalizedI);

            samplePosition = samplePosition * scale;
            math::XMStoreFloat4(&m_ssaoBufferData.sampleVectors[i], samplePosition);
        }

        m_ssaoBuffer = graphicsDevice->createBuffer<interlop::SSAOBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"SSAO Buffer",
        });

        m_ssaoBufferData.bias = 0.025f;
        m_ssaoBufferData.radius = 1.625f;
        m_ssaoBufferData.power = 1.0f;
        m_ssaoBufferData.occlusionMultiplier = 1.311f;

        // Create the random rotation texture (a 4/4 texture in range ([-1, 1], [-1, 1]).
        m_ssaoBufferData.noiseScaleDimensions = {width / static_cast<float>(interlop::NOISE_TEXTURE_DIMENSIONS), height / static_cast<float>(interlop::NOISE_TEXTURE_DIMENSIONS)};

        std::array<math::XMFLOAT2, interlop::NOISE_TEXTURE_DIMENSIONS * interlop::NOISE_TEXTURE_DIMENSIONS> randomRotationTextureData{};
        for (const uint32_t i : std::views::iota(0u, randomRotationTextureData.size()))
        {
            randomRotationTextureData[i] =
                math::XMFLOAT2(minusOneToOneDistribution(generator), minusOneToOneDistribution(generator));
        }

        m_randomRotationTexture = graphicsDevice->createTexture(
            gfx::TextureCreationDesc{
                .usage = gfx::TextureUsage::TextureFromData,
                .width = interlop::NOISE_TEXTURE_DIMENSIONS,
                .height = interlop::NOISE_TEXTURE_DIMENSIONS,
                .format = DXGI_FORMAT_R8G8_SNORM,
                .bytesPerPixel = 2u,
                .name = L"Random Rotation Texture",
            },
            randomRotationTextureData.data());

        // Create render targets and pipeline states.
        m_ssaoTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::UAVTexture,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R32_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            .name = L"SSAO Texture",
        });

        m_ssaoPipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/RenderPass/SSAOPass.hlsl",
            .pipelineName = L"SSAO Pipeline State",
        });

        m_blurSSAOTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::UAVTexture,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R32_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            .name = L"SSAO Blur Texture",
        });

        m_boxBlurPipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/PostProcessing/BoxBlur.hlsl",
            .pipelineName = L"Box Blur Pipeline State",
        });

        m_ssaoBuffer.update(&m_ssaoBufferData);
    }

    void SSAOPass::render(gfx::GraphicsContext* const graphicsContext, interlop::SSAORenderResources& renderResources,
                          const uint32_t width, const uint32_t height)
    {
        m_ssaoBufferData.screenDimensions = {static_cast<float>(width), static_cast<float>(height)};
        m_ssaoBufferData.noiseScaleDimensions = {width / static_cast<float>(interlop::NOISE_TEXTURE_DIMENSIONS), height / static_cast<float>(interlop::NOISE_TEXTURE_DIMENSIONS)};

        m_ssaoBuffer.update(&m_ssaoBufferData);

        // Setup the SSAO texture.
        {
            graphicsContext->setComputePipelineState(m_ssaoPipelineState);
                
            renderResources.randomRotationTextureIndex = m_randomRotationTexture.srvIndex;
            renderResources.ssaoBufferIndex = m_ssaoBuffer.cbvIndex;
            renderResources.outputTextureIndex = m_ssaoTexture.uavIndex;                

            graphicsContext->set32BitComputeConstants(&renderResources);

            graphicsContext->dispatch(std::max(width / 12u, 1u), std::max(height / 8u, 1u), 1);
        }

        graphicsContext->addResourceBarrier(m_ssaoTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        graphicsContext->executeResourceBarriers();

        // Blur the ssao texture.
        {
            graphicsContext->setComputePipelineState(m_boxBlurPipelineState);
           
            const interlop::BoxBlurRenderResources blurRenderResources = {
                .textureIndex = m_ssaoTexture.srvIndex,
                .outputTextureIndex = m_blurSSAOTexture.uavIndex,
            };

            graphicsContext->set32BitComputeConstants(&blurRenderResources);

            graphicsContext->dispatch(std::max(width / 12u, 1u), std::max(height / 8u, 1u), 1);
        }
    }
} // namespace helios::rendering
