#include "Rendering/BloomPass.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

namespace helios::rendering
{
    BloomPass::BloomPass(gfx::GraphicsDevice* const graphicsDevice, const uint32_t width, const uint32_t height)
    {
        // Create downsampling resources.
        m_bloomDownSamplePipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/RenderPass/BloomDownSample.hlsl",
            .pipelineName = L"Bloom DownSample Pipeline State",
        });

        m_bloomDownSampleTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::UAVTexture,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            .mipLevels = interlop::BLOOM_PASSES,
            .name = L"Bloom Downsample Texture",
        });

        // Create upsampling resources.
        m_bloomUpSamplePipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/RenderPass/BloomUpSample.hlsl",
            .pipelineName = L"Bloom UpSample Pipeline State",
        });

        m_bloomUpSampleTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::UAVTexture,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            .mipLevels = interlop::BLOOM_PASSES,
            .name = L"Bloom UpSample Texture",
        });

        // Create Bloom buffer.
        m_bloomBuffer = graphicsDevice->createBuffer<interlop::BloomBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"Bloom Buffer",
        });

        m_bloomBufferData.threshHold = 1.0f;
        m_bloomBufferData.radius = 1.0f;

        m_bloomBuffer.update(&m_bloomBufferData);

        // Create Bloom extraction texture and pipeline state.
        m_extractionTexture = graphicsDevice->createTexture(gfx::TextureCreationDesc{
            .usage = gfx::TextureUsage::UAVTexture,
            .width = width,
            .height = height,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .optionalInitialState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            .mipLevels = 1u,
            .name = L"Bloom Extraction Texture",
        });

        m_extractionPipelineState = graphicsDevice->createPipelineState(gfx::ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/RenderPass/BloomExtract.hlsl",
            .pipelineName = L"Bloom Extraction Pipeline State",
        });
    }

    void BloomPass::render(gfx::GraphicsContext* const graphicsContext, gfx::Texture& texture, const uint32_t width,
                           const uint32_t height)
    {
        m_bloomBuffer.update(&m_bloomBufferData);

        {
            graphicsContext->setComputeRootSignatureAndPipeline(m_extractionPipelineState);

            interlop::BloomExtractRenderResources renderResources = {
                .inputTextureIndex = texture.srvIndex,
                .outputTextureIndex = m_extractionTexture.uavIndex,
                .bloomBufferIndex = m_bloomBuffer.cbvIndex,
            };

            graphicsContext->set32BitComputeConstants(&renderResources);
            graphicsContext->dispatch(std::max((uint32_t)std::ceil(width / 8.0f), 1u),
                                      std::max((uint32_t)std::ceil(height / 8.0f), 1u), 1);
            // Separate high intensity pixel's from the texture using the bloom extract pipeline.
            graphicsContext->addResourceBarrier(m_extractionTexture.allocation.resource.Get(),
                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            graphicsContext->executeResourceBarriers();
        }

        // Perform downsampling (karis average for first downsample and 13 bilinear taps for subsequence bloom down
        // sample passes.
        {
            graphicsContext->setComputeRootSignatureAndPipeline(m_bloomDownSamplePipelineState);

            for (const uint32_t i : std::views::iota(0u, interlop::BLOOM_PASSES))
            {
                // When i is 0 (first bloom pass), perform karis average and use extraction texture as the resource.
                // In subsequent passes, use the bloom downsample texture at mip level 'i' as UAV, and mip level 'i - 1'
                // as SRV, and perform the 13 biliner fetch downsampling.
                if (i != 0)
                {
                    // The last subresource / mip level will NEVER be in non pixel shader resource state, so we skip
                    // that transition.
                    if (i != interlop::BLOOM_PASSES - 1)
                    {
                        graphicsContext->addResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
                            m_bloomDownSampleTexture.allocation.resource.Get(),
                            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i));
                    }

                    graphicsContext->addResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
                        m_bloomDownSampleTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, i - 1));

                    graphicsContext->executeResourceBarriers();
                }
                else if (i == 0)
                {
                    // Sort of a base case for when bloom pass index != 0.
                    graphicsContext->addResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
                        m_bloomDownSampleTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0));

                    graphicsContext->executeResourceBarriers();
                }

                const uint32_t destinationWidth = std::max<uint32_t>((uint32_t)width >> (i), 1u);
                const uint32_t destinationHeight = std::max<uint32_t>((uint32_t)height >> (i), 1u);

                // i -> 0, use sourceMipIndex as texture passed into this function.
                // i -> 1, use bloom texture srv source mip level as 0, destination index as 1
                interlop::BloomDownSampleRenderResources bloomDownSampleRenderResources = {
                    .inputTextureIndex = m_bloomDownSampleTexture.srvIndex,
                    .inputTextureMipLevel = i - 1,
                    .outputTextureIndex = m_bloomDownSampleTexture.uavIndex + i,
                    .bloomPassIndex = i,
                    .texelSize = {1.0f / destinationWidth, 1.0f / destinationHeight},
                };

                if (i == 0u)
                {
                    bloomDownSampleRenderResources.inputTextureIndex = m_extractionTexture.srvIndex;
                    bloomDownSampleRenderResources.inputTextureMipLevel = 0;
                }

                graphicsContext->set32BitComputeConstants(&bloomDownSampleRenderResources);
                graphicsContext->dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u),
                                          std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1);

                // Wait for all UAV accesses to be completed.
                graphicsContext->addResourceBarrier(m_bloomDownSampleTexture.allocation.resource.Get());
                graphicsContext->executeResourceBarriers();
            }
        }

        // Perform upsampling.
        // If the lowest downsample texture is E and highest is A,
        // UpSample(E) = E
        // UpSample(D) = DownSample(D) + UpSample(E)
        // UpSample(C) = DownSample(C) + UpSample(D)
        {
            graphicsContext->setComputeRootSignatureAndPipeline(m_bloomUpSamplePipelineState);

            for (int32_t i = interlop::BLOOM_PASSES - 1; i >= 0; --i)
            {
                graphicsContext->addResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
                    m_bloomUpSampleTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i));

                if (i != interlop::BLOOM_PASSES - 1)
                {
                    graphicsContext->addResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
                        m_bloomUpSampleTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, i + 1));
                }

                graphicsContext->executeResourceBarriers();

                const uint32_t destinationWidth = std::max<uint32_t>((uint32_t)width >> (i), 1u);
                const uint32_t destinationHeight = std::max<uint32_t>((uint32_t)height >> (i), 1u);
                // The lowest mip level will be copied onto the up sampling texture in the first pass (when
                // bloomPassIndex == interlop::BLOOM_PASSES - 1).

                interlop::BloomUpSampleRenderResources bloomUpSampleRenderResources = {
                    .inputPreviousUpSampleSrvIndex = m_bloomUpSampleTexture.srvIndex,
                    .inputPreviousUpSampleMipLevel = static_cast<uint32_t>(i + 1),
                    .inputCurrentDownSampleUavIndex = m_bloomDownSampleTexture.uavIndex + i,
                    .outputCurrentUpSampleMipIndex = m_bloomUpSampleTexture.uavIndex + i,
                    .bloomPassIndex = static_cast<uint32_t>(i),
                    .bloomBufferIndex = m_bloomBuffer.cbvIndex,
                    .texelSize = {1.0f / (destinationWidth), 1.0f / (destinationHeight)},
                };

                graphicsContext->set32BitComputeConstants(&bloomUpSampleRenderResources);
                graphicsContext->dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u),
                                          std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1);

                // Wait for all UAV accesses to be completed.
                graphicsContext->addResourceBarrier(m_bloomUpSampleTexture.allocation.resource.Get());
                graphicsContext->executeResourceBarriers();
            }

            // Just subresource 0 needs to be transitioned to all shader resource as everything else is *already* in
            // that state.
            graphicsContext->addResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
                m_bloomUpSampleTexture.allocation.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, 0u));

            graphicsContext->executeResourceBarriers();
        }
    }
} // namespace helios::rendering