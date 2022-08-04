#include "Pch.hpp"

#include "SkyBox.hpp"

namespace helios::scene
{
	SkyBox::SkyBox(gfx::Device* device, const SkyBoxCreationDesc& skyBoxCreationDesc)
	{
		// Create environment textures.
		gfx::TextureCreationDesc equirectTextureCreationDesc
		{
			.usage = gfx::TextureUsage::HDRTextureFromPath,
			.format = skyBoxCreationDesc.format,
			.name = skyBoxCreationDesc.name + L"skyboxEquirect Texture",
			.path = skyBoxCreationDesc.equirectangularTexturePath
		};

		std::unique_ptr<gfx::Texture> equirectTexture = std::make_unique<gfx::Texture>(device->CreateTexture(equirectTextureCreationDesc));

		// Create cube map texture.
		gfx::TextureCreationDesc environmentCubeMapTextureCreationDesc
		{
			.usage = gfx::TextureUsage::CubeMap,
			.dimensions = {ENVIRONMENT_CUBEMAP_DIMENSION, ENVIRONMENT_CUBEMAP_DIMENSION},
			.format = skyBoxCreationDesc.format,
			.mipLevels = 6u,
			.depthOrArraySize = 6u,
			.name = skyBoxCreationDesc.name,
		};

		mSkyBoxTexture = std::make_unique<gfx::Texture>(device->CreateTexture(environmentCubeMapTextureCreationDesc));

		// Create pipeline states.
		gfx::ComputePipelineStateCreationDesc cubeMapFromEquirectPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/CubeMapFromEquirectTextureCS.cso",
			.pipelineName = L"Final Render Target Pipeline"
		};

		mCubeMapFromEquirectPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(cubeMapFromEquirectPipelineStateCreationDesc));

		// Generate cube map.
		auto computeContext = device->GetComputeContext();

		computeContext->AddResourceBarrier(mSkyBoxTexture->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext->ExecuteResourceBarriers();

		computeContext->SetComputePipelineState(mCubeMapFromEquirectPipelineState.get());

		uint32_t size{ ENVIRONMENT_CUBEMAP_DIMENSION };
		for (uint32_t i : std::views::iota(0u, 6u))
		{
			gfx::UavCreationDesc uavCreationDesc
			{
				.uavDesc =
				{
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
					.Texture2DArray
					{
						.MipSlice = i,
						.FirstArraySlice = 0u,
						.ArraySize = 6u,
					}
				}
			};

			uint32_t uavIndex = device->CreateUav(uavCreationDesc, mSkyBoxTexture->GetResource());

			CubeFromEquirectRenderResources cubeFromEquirectRenderResources
			{
				.textureIndex = equirectTexture->srvIndex,
				.outputTextureIndex = uavIndex
			};

			computeContext->Set32BitComputeConstants(&cubeFromEquirectRenderResources);

			const uint32_t numGroups = std::max(1u, size / 32u);

			computeContext->Dispatch(numGroups, numGroups, 6u);

			size /= 2;
		}
		
		computeContext->AddResourceBarrier(mSkyBoxTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
		computeContext->ExecuteResourceBarriers();

		// Generate mips for all the cube faces.
		device->GetMipMapGenerator()->GenerateMips(mSkyBoxTexture.get());

		device->ExecuteContext(std::move(computeContext));	

		// Create skybox model.

		scene::ModelCreationDesc cubeCreationDesc
		{
			.modelPath = SKYBOX_MODEL_PATH,
			.modelName = L"Sky Box Model",
		};

		mSkyBoxModel = std::make_unique<scene::Model>(device, cubeCreationDesc);
	}

	void SkyBox::Render(const gfx::GraphicsContext* graphicsContext, SkyBoxRenderResources& renderResources)
	{
		renderResources.textureIndex = mSkyBoxTexture->srvIndex;

		mSkyBoxModel->Render(graphicsContext, renderResources);
	}
}


