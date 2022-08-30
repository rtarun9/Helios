#include "BloomPass.hpp"

namespace helios::gfx
{
	BloomPass::BloomPass(const gfx::Device* device, const Uint2& dimensions)
	{
		// Create Bloom buffer.
		gfx::BufferCreationDesc bloomBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Bloom Buffer"
		};

		mBloomBufferData =
		{
			.threshHoldValue = 0.9f
		};

		mBloomBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<BloomBuffer>(bloomBufferCreationDesc));

		// Create Bloom pipeline state(s).
		gfx::ComputePipelineStateCreationDesc bloomPipelineState
		{
			.csShaderPath = L"Shaders/RenderPass/BloomCS.cso",
			.pipelineName = L"Bloom Pipeline State"
		};

		mBloomPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(bloomPipelineState));

		// Create the texture (will act as threshold capture and also for mip map result texture).
		// Will generate all mips (up until dimension 1x1)
		gfx::TextureCreationDesc textureCreationDesc
		{
			.usage = gfx::TextureUsage::UAVTexture,
			.dimensions = dimensions,
			.format = DXGI_FORMAT_R11G11B10_FLOAT,
			.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(dimensions.x, dimensions.y))) + 1),
			.name = L"Bloom Pass Texture",
		};

		mBloomTextures = std::make_unique<gfx::Texture>(device->CreateTexture(textureCreationDesc));
	
		// Create mip map buffer (to be passed used when mip maps are being created).
		gfx::BufferCreationDesc mipMapBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Mip Map Buffer Creation Desc Bloom Pass"
		};

		mMipMapBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<MipMapGenerationBuffer>(mipMapBufferCreationDesc, std::span<MipMapGenerationBuffer, 0u>{}));

		// Create the SRV's and UAV's for bloom pass (mip map generation part).
		SrvCreationDesc srvCreationDesc
		{
			.srvDesc
			{
				.Format = textureCreationDesc.format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D
				{
					.MipLevels = textureCreationDesc.mipLevels
				}
			}
		};

		mSrvIndex = device->CreateSrv(srvCreationDesc, mBloomTextures->GetResource());


		// Create the UAV's for the bloom pass.
		for (uint32_t uav : std::views::iota(0u, static_cast<uint32_t>(textureCreationDesc.mipLevels)))
		{
			UavCreationDesc uavCreationDesc
			{
				.uavDesc
				{
					.Format = gfx::Texture::GetNonSRGBFormat(textureCreationDesc.format),
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = uav,
						.PlaneSlice = 0u
					}
				}
			};

			//  Reading from a SRV/UAV mapped to a null resource will return black and writing to a UAV mapped to a null resource will have no effect (from 3DGEP).
			mMipUavIndices.push_back(device->CreateUav(uavCreationDesc, mBloomTextures->GetResource()));
		}
	}

	void BloomPass::OnResize(const gfx::Device* device, const Uint2& dimensions)
	{
		// note(rtarun9) : Todo.
	}

	void BloomPass::Render(gfx::Device* const device, gfx::RenderTarget* renderTarget, gfx::ComputeContext* computeContext)
	{
		BloomPassRenderResources renderResourcess
		{
			.inputTextureIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(renderTarget),
			.outputTextureIndex = mMipUavIndices[0]
		};


		computeContext->Set32BitComputeConstants(&renderResourcess);

		computeContext->Dispatch(mBloomTextures->dimensions.x / 8u, mBloomTextures->dimensions.y / 8u, 1u);
		
		device->GetMipMapGenerator()->GenerateMips(mBloomTextures.get(), mSrvIndex, mMipUavIndices, mMipMapBuffer.get());
	}
}