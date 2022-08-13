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
			.mipLevels = 6u,
			.name = skyBoxCreationDesc.name + L" Skybox Equirect Texture",
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

		// Create irradiance map texture.
		gfx::TextureCreationDesc diffuseIrradianceCubeMapTextureCreationDesc
		{
			.usage = gfx::TextureUsage::CubeMap,
			.dimensions = {DIFFUSE_IRRADIANCE_MAP_TEXTURE_DIMENSION, DIFFUSE_IRRADIANCE_MAP_TEXTURE_DIMENSION},
			.format = skyBoxCreationDesc.format,
			.mipLevels = 1u,
			.depthOrArraySize = 6u,
			.name = skyBoxCreationDesc.name + L" Irradiance Map",
		};

		mDiffuseIrradianceMapTexture = std::make_unique<gfx::Texture>(device->CreateTexture(diffuseIrradianceCubeMapTextureCreationDesc));

		// Create pipeline states.
		gfx::ComputePipelineStateCreationDesc cubeMapFromEquirectPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/SkyBox/CubeMapFromEquirectTextureCS.cso",
			.pipelineName = L"Cube Map From Equirect Pipeline"
		};

		mCubeMapFromEquirectPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(cubeMapFromEquirectPipelineStateCreationDesc));

		gfx::ComputePipelineStateCreationDesc diffuseIrradianceMapPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/IBL/DiffuseIrradianceCS.cso",
			.pipelineName = L"Diffuse IrradiancePipeline"
		};

		mDiffuseIrradianceMapPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(diffuseIrradianceMapPipelineStateCreationDesc));

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

		// Run compute shader to generate irradiance map from sky box texture.
		auto irradianceMapComputeContext = device->GetComputeContext();
		irradianceMapComputeContext->AddResourceBarrier(mDiffuseIrradianceMapTexture->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		irradianceMapComputeContext->ExecuteResourceBarriers();

		irradianceMapComputeContext->SetComputePipelineState(mDiffuseIrradianceMapPipelineState.get());
		DiffuseIrradianceRenderResources diffuseIrradianceRenderResources
		{
			.cubeMapTextureIndex = mSkyBoxTexture->srvIndex,
			.ouputIrradianceMapIndex = mDiffuseIrradianceMapTexture->uavIndex
		};

		irradianceMapComputeContext->Set32BitComputeConstants(&diffuseIrradianceRenderResources);

		irradianceMapComputeContext->Dispatch(DIFFUSE_IRRADIANCE_MAP_TEXTURE_DIMENSION / 32u, DIFFUSE_IRRADIANCE_MAP_TEXTURE_DIMENSION / 32u, 6u);
		irradianceMapComputeContext->AddResourceBarrier(mDiffuseIrradianceMapTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
		irradianceMapComputeContext->ExecuteResourceBarriers();

		device->ExecuteContext(std::move(irradianceMapComputeContext));

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
		renderResources.textureIndex = mDiffuseIrradianceMapTexture->srvIndex;

		mSkyBoxModel->Render(graphicsContext, renderResources);
	}
}


