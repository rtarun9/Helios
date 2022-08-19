#include "SkyBox.hpp"

namespace helios::scene
{
	SkyBox::SkyBox(gfx::Device* device, const SkyBoxCreationDesc& skyBoxCreationDesc) 
	{
		// Create equirectangular HDR texture and a environment cube map texture with 6 faces.

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
		gfx::TextureCreationDesc irradianceCubeMapTextureCreationDesc
		{
			.usage = gfx::TextureUsage::CubeMap,
			.dimensions = {IRRADIANCE_MAP_TEXTURE_DIMENSION, IRRADIANCE_MAP_TEXTURE_DIMENSION},
			.format = skyBoxCreationDesc.format,
			.mipLevels = 1u,
			.depthOrArraySize = 6u,
			.name = skyBoxCreationDesc.name + L" Irradiance Map",
		};

		mIrradianceMapTexture = std::make_unique<gfx::Texture>(device->CreateTexture(irradianceCubeMapTextureCreationDesc));

		// Create Pre Filter map texture.
		gfx::TextureCreationDesc preFilterMapTextureCreationDesc
		{
			.usage = gfx::TextureUsage::CubeMap,
			.dimensions = {PREFILTER_MAP_TEXTURE_DIMENSION, PREFILTER_MAP_TEXTURE_DIMENSION},
			.format = skyBoxCreationDesc.format,
			.mipLevels = 7u,
			.depthOrArraySize = 6u,
			.name = skyBoxCreationDesc.name + L" Pre Filter Map",
		};

		mPreFilterTexture = std::make_unique<gfx::Texture>(device->CreateTexture(preFilterMapTextureCreationDesc));
		
		// Create BRDF LUT map texture.
		gfx::TextureCreationDesc brdfLutTextureCreationDesc
		{
			.usage = gfx::TextureUsage::CubeMap,
			.dimensions = {BRDF_LUT_TEXTURE_DIMENSION, BRDF_LUT_TEXTURE_DIMENSION},
			.format = skyBoxCreationDesc.format,
			.mipLevels = 1u,
			.depthOrArraySize = 1u,
			.name = skyBoxCreationDesc.name + L" Pre Filter Map",
		};

		mBRDFLutTexture = std::make_unique<gfx::Texture>(device->CreateTexture(brdfLutTextureCreationDesc));

		// Create pipeline states.
		gfx::ComputePipelineStateCreationDesc cubeMapFromEquirectPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/SkyBox/CubeMapFromEquirectTextureCS.cso",
			.pipelineName = L"Cube Map From Equirect Pipeline"
		};

		mCubeMapFromEquirectPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(cubeMapFromEquirectPipelineStateCreationDesc));

		gfx::ComputePipelineStateCreationDesc diffuseIrradianceMapPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/IBL/IrradianceCS.cso",
			.pipelineName = L"Irradiance Pipeline"
		};

		mIrradianceMapPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(diffuseIrradianceMapPipelineStateCreationDesc));

		gfx::ComputePipelineStateCreationDesc preFilterMapPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/IBL/PrefilterCS.cso",
			.pipelineName = L"Pre Filter Pipeline"
		};

		mPrefilterMapPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(preFilterMapPipelineStateCreationDesc));

		gfx::ComputePipelineStateCreationDesc brdfLutPipelineStateCreationDesc
		{
			.csShaderPath = L"Shaders/IBL/BRDFLutCS.cso",
			.pipelineName = L" BRDF LUT Pipeline"
		};

		mBRDFLutPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(brdfLutPipelineStateCreationDesc));

		// Generate cube map from single equirectangular texture
		{
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
					.textureIndex = gfx::Texture::GetSrvIndex(equirectTexture.get()),
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
		}

		// Run compute shader to generate irradiance map from sky box texture.
		{
			auto computeContext = device->GetComputeContext();

			computeContext->AddResourceBarrier(mIrradianceMapTexture->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext->ExecuteResourceBarriers();

			computeContext->SetComputePipelineState(mIrradianceMapPipelineState.get());

			IrradianceRenderResources diffuseIrradianceRenderResources
			{
				.skyBoxTextureIndex = gfx::Texture::GetSrvIndex(mSkyBoxTexture.get()),
				.ouputIrradianceMapIndex = gfx::Texture::GetUavIndex(mIrradianceMapTexture.get())
			};

			computeContext->Set32BitComputeConstants(&diffuseIrradianceRenderResources);

			computeContext->Dispatch(IRRADIANCE_MAP_TEXTURE_DIMENSION / 32u, IRRADIANCE_MAP_TEXTURE_DIMENSION / 32u, 6u);
			computeContext->AddResourceBarrier(mIrradianceMapTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
			computeContext->ExecuteResourceBarriers();

			device->ExecuteContext(std::move(computeContext));
		}
		
		// Run compute shader to generate prefilter map from skybox texture.
		{
			auto computeContext = device->GetComputeContext();

			computeContext->AddResourceBarrier(mPreFilterTexture->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext->ExecuteResourceBarriers();

			computeContext->SetComputePipelineState(mPrefilterMapPipelineState.get());
			
			uint32_t size{ PREFILTER_MAP_TEXTURE_DIMENSION };
			for (uint32_t i = 0; i < 7u; i++)
			{
				gfx::UavCreationDesc uavCreationDesc
				{
					.uavDesc
					{
						.Format = skyBoxCreationDesc.format,
						.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
						.Texture2DArray
						{
							.MipSlice = i,
							.FirstArraySlice = 0u,
							.ArraySize = 6u
						}
					}
				};

				uint32_t uavIndex = device->CreateUav(uavCreationDesc, mPreFilterTexture->GetResource());

				PreFilterCubeMapRenderResources preFilterCubeMapRenderResources
				{
					.skyBoxTextureIndex = gfx::Texture::GetSrvIndex(mSkyBoxTexture.get()),
					.outputPreFilteredCubeMapIndex = uavIndex,
					.mipLevel = i
				};

				const uint32_t numGroups = std::max(1u, size / 32u);

				computeContext->Set32BitComputeConstants(&preFilterCubeMapRenderResources);

				computeContext->Dispatch(numGroups, numGroups, 6u);
				size /= 2;
			}

			computeContext->AddResourceBarrier(mPreFilterTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
			computeContext->ExecuteResourceBarriers();

			device->ExecuteContext(std::move(computeContext));
		}

		// Run compute shader to generate BRDF Lut.
		{
			auto computeContext = device->GetComputeContext();

			computeContext->AddResourceBarrier(mBRDFLutTexture->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext->ExecuteResourceBarriers();

			computeContext->SetComputePipelineState(mBRDFLutPipelineState.get());

			BRDFLutRenderResources brdfLutRenderResources
			{
				.lutTextureIndex = gfx::Texture::GetUavIndex(mBRDFLutTexture.get())
			};

			computeContext->Set32BitComputeConstants(&brdfLutRenderResources);

			computeContext->Dispatch(BRDF_LUT_TEXTURE_DIMENSION / 32u, BRDF_LUT_TEXTURE_DIMENSION / 32u, 1u);

			computeContext->AddResourceBarrier(mBRDFLutTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
			computeContext->ExecuteResourceBarriers();

			device->ExecuteContext(std::move(computeContext));
		}

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
		renderResources.textureIndex = gfx::Texture::GetSrvIndex(mSkyBoxTexture.get());

		mSkyBoxModel->Render(graphicsContext, renderResources);
	}
}


