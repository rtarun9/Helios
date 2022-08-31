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
			.threshHoldParams = {1.0f, 0.1f}
		};

		mBloomBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<BloomBuffer>(bloomBufferCreationDesc));

		// Create Bloom pipeline state(s).
		gfx::ComputePipelineStateCreationDesc bloomPipelineState
		{
			.csShaderPath = L"Shaders/RenderPass/BloomCS.cso",
			.pipelineName = L"Bloom Pipeline State"
		};

		mBloomPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(bloomPipelineState));

		gfx::TextureCreationDesc preFilterTextureCreationDesc
		{
			.usage = gfx::TextureUsage::UAVTexture,
			.dimensions = dimensions,
			.format = DXGI_FORMAT_R11G11B10_FLOAT,
			.mipLevels = BLOOM_MIP_LEVELS,
			.name = L"Bloom Pre Filter Pass Texture",
		};

		mPreFilterBloomTexture = std::make_unique<gfx::Texture>(device->CreateTexture(preFilterTextureCreationDesc));
	
		mSrvIndex = gfx::Texture::GetSrvIndex(mPreFilterBloomTexture.get());

		mPreFilterUavIndex = gfx::Texture::GetUavIndex(mPreFilterBloomTexture.get());

		gfx::TextureCreationDesc upDownSamplingTextureCreationDesc
		{
			.usage = gfx::TextureUsage::UAVTexture,
			.dimensions = dimensions,
			.format = DXGI_FORMAT_R11G11B10_FLOAT,
			.mipLevels = BLOOM_MIP_LEVELS ,
			.name = L"Bloom Pass Up / Down Sampling Texture",
		};

		mUpDownSampledBloomTextures = std::make_unique<gfx::Texture>(device->CreateTexture(upDownSamplingTextureCreationDesc));

		mMipUavIndices.reserve(BLOOM_MIP_LEVELS);

		// Create the UAV's for the bloom pass.

		for (uint32_t uav : std::views::iota(0u, BLOOM_MIP_LEVELS))
		{
			UavCreationDesc uavCreationDesc
			{
				.uavDesc
				{
					.Format = gfx::Texture::GetNonSRGBFormat(upDownSamplingTextureCreationDesc.format),
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = uav,
						.PlaneSlice = 0u
					}
				}
			};

			//  Reading from a SRV/UAV mapped to a null resource will return black and writing to a UAV mapped to a null resource will have no effect (from 3DGEP).
			mMipUavIndices.push_back(device->CreateUav(uavCreationDesc, mUpDownSampledBloomTextures->GetResource()));
		}
	}

	void BloomPass::OnResize(const gfx::Device* device, const Uint2& dimensions)
	{
		// note(rtarun9) : Todo.
	}

	void BloomPass::Render(gfx::Device* const device, gfx::RenderTarget* renderTarget)
	{
		for (uint32_t passes : std::views::iota(0u, BLOOM_PASSES))
		{
			// Do the Prefilter pass first.
			uint32_t srcWidth = mPreFilterBloomTexture->dimensions.x;
			uint32_t srcHeight = mPreFilterBloomTexture->dimensions.y;

			mBloomBufferData.shaderUsage = BloomShaderUsage::PreFilter;
			mBloomBufferData.texelSize = { 1.0f / srcWidth, 1.0f / srcHeight };

			mBloomBuffer->Update(&mBloomBufferData);

			BloomPassRenderResources renderResourcess
			{
				.inputTextureIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(renderTarget),
				.outputTextureIndex = mPreFilterUavIndex,
				.bloomBufferIndex = gfx::Buffer::GetCbvIndex(mBloomBuffer.get())
			};

			auto computeContext = device->GetComputeContext(mBloomPipelineState.get());

			computeContext->Set32BitComputeConstants(&renderResourcess);

			computeContext->Dispatch(srcWidth / 8u, srcHeight / 8u, 1u);
			device->GetComputeCommandQueue()->ExecuteAndFlush(computeContext->GetCommandList());

			// Copy the prefilter texture into the first (0th) mip level of the up/down sampling textures.
			auto commandList = device->GetGraphicsContext();
			commandList->CopyResource(mPreFilterBloomTexture->GetResource(), mUpDownSampledBloomTextures->GetResource());
			device->GetGraphicsCommandQueue()->ExecuteAndFlush(commandList->GetCommandList());

			// Do the down sampling.

			for (uint32_t mipLevel : std::views::iota(0u, BLOOM_MIP_LEVELS - 1u))
			{
				uint64_t sourceWidth = srcWidth >> mipLevel;
				uint64_t sourceHeight = srcHeight >> mipLevel;

				uint32_t destinationWidth = std::max<uint32_t>((uint32_t)sourceWidth >> (1), 1u);
				uint32_t destinationHeight = std::max<uint32_t>((uint32_t)sourceHeight >> (1), 1u);

				mBloomBufferData.shaderUsage = mipLevel == 0u ? BloomShaderUsage::FirstDownsample : BloomShaderUsage::Downsample;
				mBloomBufferData.texelSize = { 1.0f / destinationWidth, 1.0f / destinationHeight };

				mBloomBuffer->Update(&mBloomBufferData);

				auto downsampleComputeContext = device->GetComputeContext(mBloomPipelineState.get());

				BloomPassRenderResources renderResourcess
				{
					.inputTextureIndex = mMipUavIndices[mipLevel],
					.outputTextureIndex = mMipUavIndices[mipLevel + 1u],
					.bloomBufferIndex = gfx::Buffer::GetCbvIndex(mBloomBuffer.get())
				};

				downsampleComputeContext->Set32BitComputeConstants(&renderResourcess);

				downsampleComputeContext->Dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u), std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1u);
				device->GetComputeCommandQueue()->ExecuteAndFlush(downsampleComputeContext->GetCommandList());
			}

			// Do the upsampling such that final result is in the first UAV index for up / down sampling texture Index.
			for (uint32_t mipLevel = BLOOM_MIP_LEVELS - 1u; mipLevel > 0; --mipLevel)
			{
				uint64_t sourceWidth = srcWidth >> mipLevel;
				uint64_t sourceHeight = srcHeight >> mipLevel;

				uint32_t destinationWidth = std::min<uint32_t>((uint32_t)sourceWidth << (1), srcWidth);
				uint32_t destinationHeight = std::min<uint32_t>((uint32_t)sourceHeight << (1), srcHeight);

				mBloomBufferData.shaderUsage = BloomShaderUsage::Upsample;
				mBloomBufferData.texelSize = { 1.0f / destinationWidth, 1.0f / destinationHeight };

				mBloomBuffer->Update(&mBloomBufferData);

				auto upSampleComputeContext = device->GetComputeContext(mBloomPipelineState.get());

				BloomPassRenderResources renderResourcess
				{
					.inputTextureIndex = mMipUavIndices[mipLevel],
					.outputTextureIndex = mMipUavIndices[mipLevel - 1u],
					.bloomBufferIndex = gfx::Buffer::GetCbvIndex(mBloomBuffer.get())
				};

				upSampleComputeContext->Set32BitComputeConstants(&renderResourcess);

				upSampleComputeContext->Dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u), std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1u);
				device->GetComputeCommandQueue()->ExecuteAndFlush(upSampleComputeContext->GetCommandList());
			}
		}
	}
}