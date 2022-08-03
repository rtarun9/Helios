#include "Pch.hpp"

#include "MipMapGenerator.hpp"

#include "Device.hpp"

#include "Common/ConstantBuffers.hlsli"

namespace helios::gfx
{
	MipMapGenerator::MipMapGenerator(gfx::Device& device): mDevice(device)
	{
		ComputePipelineStateCreationDesc pipelineCreationDesc
		{
			.csShaderPath = L"Shaders/GenerateMipsCS.cso",
			.pipelineName = L"Mip Map Generation Pipeline"
		};

		mMipMapPipelineState = std::make_unique<gfx::PipelineState>(device.GetDevice(), pipelineCreationDesc);	
	}

	void MipMapGenerator::GenerateMips(gfx::Texture* texture)
	{
		D3D12_RESOURCE_DESC sourceResourceDesc = texture->GetResource()->GetDesc();
		if (sourceResourceDesc.MipLevels <= 1)
		{
			return;
		}

		gfx::BufferCreationDesc mipMapBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Mip Map Buffer Creation Desc",
		};

		std::unique_ptr<gfx::Buffer> mipMapBuffer = std::make_unique<gfx::Buffer>(mDevice.CreateBuffer<MipMapGenerationBuffer>(mipMapBufferCreationDesc, std::span<MipMapGenerationBuffer, 0u>{}));

		for (uint32_t i : std::views::iota(0u, sourceResourceDesc.MipLevels - 1u))
		{
			uint32_t destinationWidth = std::max<UINT>(sourceResourceDesc.Width >> (i + 1), 1);
			uint32_t destinationHeight = std::max<UINT>(sourceResourceDesc.Height >> (i + 1), 1);

			SrvCreationDesc srvCreationDesc
			{
				.srvDesc
				{
					.Format = sourceResourceDesc.Format,
					.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Texture2D
					{
						.MostDetailedMip = i,
						.MipLevels = 1u
					}
				}
			};
			
			uint32_t srvIndex = mDevice.CreateSrv(srvCreationDesc, texture->GetResource());
			
			// NOTE : UAV's have a limited set of formats they can use.
			// Refer : https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads.
			UavCreationDesc uavCreationDesc
			{
				.uavDesc 
				{
					.Format = sourceResourceDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM : sourceResourceDesc.Format,
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = i + 1,
						.PlaneSlice = 0u
					}
				}		
			};

			//  Reading from a SRV/UAV mapped to a null resource will return black and writing to a UAV mapped to a null resource will have no effect (from 3DGEP).
			uint32_t mipIndex = mDevice.CreateUav(uavCreationDesc, texture->GetResource());

			MipMapGenerationBuffer mipMapGenerationBufferData
			{
				.isSRGB = sourceResourceDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? true : false,
				.texelSize = {1.0f / destinationWidth, 1.0f / destinationHeight}
			};

			mipMapBuffer->Update(&mipMapGenerationBufferData);

			MipMapGenerationRenderResources renderResources
			{
				.sourceMipIndex = srvIndex,
				.outputMipIndex = mipIndex,
				.mipMapGenerationBufferIndex = mipMapBuffer->cbvIndex,
			};

			std::unique_ptr<ComputeContext> computeContext = mDevice.GetComputeContext();

			computeContext->SetComputePipelineState(mMipMapPipelineState.get());
			computeContext->Set32BitComputeConstants(&renderResources);

			computeContext->Dispatch(std::max(destinationWidth / 8, 1u), std::max(destinationHeight/ 8, 1u), 1);

			mDevice.GetComputeCommandQueue()->ExecuteAndFlush(computeContext->GetCommandList());
		}
	}
}

