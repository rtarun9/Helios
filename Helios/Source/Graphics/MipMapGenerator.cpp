#include "Graphics/MipMapGenerator.hpp"

#include "Graphics/ComputeContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

namespace helios::gfx
{
    MipMapGenerator::MipMapGenerator(gfx::GraphicsDevice* const graphicsDevice) : graphicsDevice(*graphicsDevice)
    {
        m_mipMapPipelineState = graphicsDevice->createPipelineState(ComputePipelineStateCreationDesc{
            .csShaderPath = L"Shaders/MipMapGeneration/GenerateMipsCS.hlsl",
            .pipelineName = L"Mip Map Generation Pipeline",
        });

        m_mipMapBuffer = graphicsDevice->createBuffer<interlop::MipMapGenerationBuffer>( // Create mip map buffer.
            gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::ConstantBuffer,
                .name = L"Mip Map Buffer",
            });
    }

    void MipMapGenerator::generateMips(gfx::Texture& texture)
    {
        D3D12_RESOURCE_DESC sourceResourceDesc = texture.allocation.resource->GetDesc();
        if (sourceResourceDesc.MipLevels <= 1)
        {
            return;
        }

        const uint32_t sourceMipSrvIndex = graphicsDevice.createSrv(
            SrvCreationDesc{
                .srvDesc{
                    .Format = sourceResourceDesc.Format,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2D = {.MipLevels = sourceResourceDesc.MipLevels},
                },
            },
            texture.allocation.resource.Get());

        // Main reference : https://www.3dgep.com/learning-directx-12-4/#CommandListGenerateMips_UAV.
        for (uint32_t srcMipLevel = 0; srcMipLevel < sourceResourceDesc.MipLevels - 1u;)
        {
            uint64_t sourceWidth = sourceResourceDesc.Width >> srcMipLevel;
            uint64_t sourceHeight = sourceResourceDesc.Height >> srcMipLevel;

            uint32_t destinationWidth = std::max<uint32_t>((uint32_t)sourceWidth >> 1u, 1u);
            uint32_t destinationHeight = std::max<uint32_t>((uint32_t)sourceHeight >> 1u, 1u);

            interlop::TextureDimensionType dimensionType{};
            if (sourceHeight % 2 == 0 && sourceWidth % 2 == 0)
            {
                dimensionType = interlop::TextureDimensionType::HeightWidthEven;
            }
            else if (sourceHeight % 2 != 0 && sourceWidth % 2 == 0)
            {
                dimensionType = interlop::TextureDimensionType::HeightOddWidthEven;
            }
            else if (sourceHeight % 2 == 0 && sourceWidth % 2 != 0)
            {
                dimensionType = interlop::TextureDimensionType::HeightEvenWidthOdd;
            }
            else
            {
                dimensionType = interlop::TextureDimensionType::HeightWidthOdd;
            }

            // At a single compute shader dispatch, we can generate atmost 4 mip maps.
            // The code below checks for in this loop iteration, how many levels can we compute, so as to have
            // subsequent mip level dimension be exactly half : exactly 50 % decrease in mip dimension. i.e number of
            // times mip can be halved until we reach a mip level where one dimension is odd. If dimension is odd,
            // texture needs to be sampled multiple times, which will be handled in a new dispatch.
            DWORD mipCount{};
            // Value of temp not required.
            _BitScanForward64(&mipCount, (destinationWidth == 1u ? destinationHeight : destinationWidth) |
                                             (destinationHeight == 1u ? destinationWidth : destinationHeight));
            mipCount = std::min<uint32_t>(4, mipCount + 1);
            mipCount = (srcMipLevel + mipCount) >= sourceResourceDesc.MipLevels
                           ? sourceResourceDesc.MipLevels - srcMipLevel - 1u
                           : mipCount;

            // NOTE : UAV's have a limited set of formats they can use.
            // Refer : https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads.
            std::array<uint32_t, 4u> mipUavs{};
            for (uint32_t uav : std::views::iota(0u, static_cast<uint32_t>(mipCount)))
            {
                UavCreationDesc uavCreationDesc{
                    .uavDesc{.Format = gfx::Texture::getNonSRGBFormat(sourceResourceDesc.Format),
                             .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                             .Texture2D{.MipSlice = uav + 1 + srcMipLevel, .PlaneSlice = 0u}}};

                //  Reading from a SRV/UAV mapped to a null resource will return black and writing to a UAV mapped to a
                //  null resource will have no effect (from 3DGEP).
                mipUavs[uav] = graphicsDevice.createUav(uavCreationDesc, texture.allocation.resource.Get());
            }

            const interlop::MipMapGenerationBuffer mipMapGenerationBufferData = {
                .isSRGB = gfx::Texture::isTextureSRGB(sourceResourceDesc.Format),
                .sourceMipLevel = srcMipLevel,
                .texelSize = {1.0f / destinationWidth, 1.0f / destinationHeight},
                .numberMipLevels = static_cast<uint32_t>(mipCount),
                .dimensionType = dimensionType,
            };

            m_mipMapBuffer.update(&mipMapGenerationBufferData);

            const interlop::MipMapGenerationRenderResources renderResources{
                .sourceMipIndex = sourceMipSrvIndex,
                .outputMip1Index = mipUavs[0],
                .outputMip2Index = mipUavs[1],
                .outputMip3Index = mipUavs[2],
                .outputMip4Index = mipUavs[3],
                .mipMapGenerationBufferIndex = m_mipMapBuffer.cbvIndex,
            };

            std::unique_ptr<ComputeContext>& computeContext = graphicsDevice.getComputeContext();
            computeContext->reset();

            computeContext->setComputeRootSignatureAndPipeline(m_mipMapPipelineState);
            computeContext->set32BitComputeConstants(&renderResources);

            computeContext->dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u),
                                     std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1);

            const std::array<Context*, 1u> contexts = {
                computeContext.get(),
            };
            graphicsDevice.getComputeCommandQueue()->executeContext(contexts);
            graphicsDevice.getComputeCommandQueue()->flush();

            srcMipLevel += static_cast<uint32_t>(mipCount);
        }
    }
} // namespace helios::gfx
