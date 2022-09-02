#include "BloomPass.hpp"

namespace helios::gfx
{
	BloomPass::BloomPass(gfx::Device* device, const Uint2& dimensions)
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
	
		mPreFilterSrvIndex = gfx::Texture::GetSrvIndex(mPreFilterBloomTexture.get());

		mPreFilterUavIndex = gfx::Texture::GetUavIndex(mPreFilterBloomTexture.get());

		// Create up and down sampling textures.
		gfx::TextureCreationDesc upSamplingTextureCreationDesc
		{
			.usage = gfx::TextureUsage::UAVTexture,
			.dimensions = {dimensions.x / 2u, dimensions.y / 2u},
			.format = DXGI_FORMAT_R11G11B10_FLOAT,
			.mipLevels = BLOOM_MIP_LEVELS ,
			.name = L"Bloom Pass Up Sampling Texture",
		};

		mUpSampledBloomTextures = std::make_unique<gfx::Texture>(device->CreateTexture(upSamplingTextureCreationDesc));
		mUpSamplingMipUavIndices.push_back(gfx::Texture::GetUavIndex(mUpSampledBloomTextures.get()));

		for (uint32_t uav : std::views::iota(1u, BLOOM_MIP_LEVELS))
		{
			UavCreationDesc uavCreationDesc
			{
				.uavDesc
				{
					.Format = gfx::Texture::GetNonSRGBFormat(upSamplingTextureCreationDesc.format),
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = uav,
						.PlaneSlice = 0u
					}
				}
			};

			//  Reading from a SRV/UAV mapped to a null resource will return black and writing to a UAV mapped to a null resource will have no effect (from 3DGEP).
			mUpSamplingMipUavIndices.push_back(device->CreateUav(uavCreationDesc, mUpSampledBloomTextures->GetResource()));
		}

		gfx::TextureCreationDesc downSamplingTextureCreationDesc
		{
			.usage = gfx::TextureUsage::UAVTexture,
			.dimensions = {dimensions.x / 2u, dimensions.y / 2u},
			.format = DXGI_FORMAT_R11G11B10_FLOAT,
			.mipLevels = BLOOM_MIP_LEVELS ,
			.name = L"Bloom Pass Down Sampling Texture",
		};

		mDownSampledBloomTextures = std::make_unique<gfx::Texture>(device->CreateTexture(downSamplingTextureCreationDesc));

		// Create uav's for down and up sampling textures.
		mDownSamplingMipUavIndices.reserve(BLOOM_MIP_LEVELS);
		mUpSamplingMipUavIndices.reserve(BLOOM_MIP_LEVELS);

		// Create the UAV's for the bloom pass.

		mDownSamplingMipUavIndices.push_back(gfx::Texture::GetUavIndex(mDownSampledBloomTextures.get()));
		for (uint32_t uav : std::views::iota(1u, BLOOM_MIP_LEVELS))
		{
			UavCreationDesc uavCreationDesc
			{
				.uavDesc
				{
					.Format = gfx::Texture::GetNonSRGBFormat(downSamplingTextureCreationDesc.format),
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = uav,
						.PlaneSlice = 0u
					}
				}
			};

			//  Reading from a SRV/UAV mapped to a null resource will return black and writing to a UAV mapped to a null resource will have no effect (from 3DGEP).
			mDownSamplingMipUavIndices.push_back(device->CreateUav(uavCreationDesc, mDownSampledBloomTextures->GetResource()));
		}
	}

	void BloomPass::OnResize(const gfx::Device* device, const Uint2& dimensions)
	{
		// note(rtarun9) : Todo.
	}

	void BloomPass::Render(gfx::Device* const device, gfx::RenderTarget* renderTarget, std::vector<std::unique_ptr<ComputeContext>>& computeContexts)
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

			auto commandList = device->GetComputeContext(mBloomPipelineState.get());

			computeContexts.push_back(std::move(commandList));

			computeContexts.back()->Set32BitComputeConstants(&renderResourcess);

			computeContexts.back()->Dispatch(srcWidth / 8u, srcHeight / 8u, 1u);

			// Do the down sampling.
			for (uint32_t mipLevel : std::views::iota(0u, BLOOM_MIP_LEVELS))
			{
				uint64_t sourceWidth = srcWidth >> mipLevel;
				uint64_t sourceHeight = srcHeight >> mipLevel;

				uint32_t destinationWidth = std::max<uint32_t>((uint32_t)sourceWidth >> (1), 1u);
				uint32_t destinationHeight = std::max<uint32_t>((uint32_t)sourceHeight >> (1), 1u);

				mBloomBufferData.shaderUsage = mipLevel == 0u ? BloomShaderUsage::Downsample : BloomShaderUsage::Downsample;
				mBloomBufferData.texelSize = { 1.0f / destinationWidth, 1.0f / destinationHeight };

				mBloomBuffer->Update(&mBloomBufferData);

				auto downsampleComputeContext = device->GetComputeContext(mBloomPipelineState.get());

				BloomPassRenderResources renderResourcess
				{
					.inputTextureIndex = mipLevel == 0u ? mPreFilterUavIndex :  mDownSamplingMipUavIndices[mipLevel - 1u],
					.outputTextureIndex = mDownSamplingMipUavIndices[mipLevel],
					.bloomBufferIndex = gfx::Buffer::GetCbvIndex(mBloomBuffer.get())
				};

				downsampleComputeContext->Set32BitComputeConstants(&renderResourcess);

				downsampleComputeContext->Dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u), std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1u);
				computeContexts.push_back(std::move(downsampleComputeContext));

				// Do the Gaussian horizontal blur.
				/*
				renderResourcess.outputTextureIndex = mDownSamplingMipUavIndices[mipLevel];
				renderResourcess.inputTextureIndex = mDownSamplingMipUavIndices[mipLevel];

				destinationWidth = std::max<uint32_t>((uint32_t)destinationWidth >> (1), 1u);
				destinationHeight = std::max<uint32_t>((uint32_t)destinationHeight >> (1), 1u);

				auto horizontalBlurComputeContext = device->GetComputeContext(mBloomPipelineState.get());

				mBloomBufferData.shaderUsage = BloomShaderUsage::GaussianBlurHorizontal;
				mBloomBufferData.texelSize = { 1.0f / destinationWidth, 1.0f / destinationHeight };

				mBloomBuffer->Update(&mBloomBufferData);

				horizontalBlurComputeContext->Set32BitComputeConstants(&renderResourcess);

				horizontalBlurComputeContext->Dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u), std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1u);
				device->GetComputeCommandQueue()->ExecuteAndFlush(horizontalBlurComputeContext->GetCommandList());

				// Do the Gaussian vertical blur.
				auto verticalBlurComputeContext = device->GetComputeContext(mBloomPipelineState.get());

				mBloomBufferData.shaderUsage = BloomShaderUsage::GaussianBlurVertical;
				mBloomBufferData.texelSize = { 1.0f / destinationWidth, 1.0f / destinationHeight };

				mBloomBuffer->Update(&mBloomBufferData);

				verticalBlurComputeContext->Set32BitComputeConstants(&renderResourcess);

				verticalBlurComputeContext->Dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u), std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1u);
				device->GetComputeCommandQueue()->ExecuteAndFlush(verticalBlurComputeContext->GetCommandList());

			*/
			}

			return;
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
					.inputTextureIndex = mipLevel == BLOOM_MIP_LEVELS - 1u ? mDownSamplingMipUavIndices[mipLevel] : mUpSamplingMipUavIndices[mipLevel],
					.outputTextureIndex = mUpSamplingMipUavIndices[mipLevel - 1u],
					.bloomBufferIndex = gfx::Buffer::GetCbvIndex(mBloomBuffer.get())
				};

				upSampleComputeContext->Set32BitComputeConstants(&renderResourcess);

				upSampleComputeContext->Dispatch(std::max((uint32_t)std::ceil(destinationWidth / 8.0f), 1u), std::max((uint32_t)std::ceil(destinationHeight / 8.0f), 1u), 1u);
				device->GetComputeCommandQueue()->ExecuteAndFlush(upSampleComputeContext->GetCommandList());
			}
		}
	}
}