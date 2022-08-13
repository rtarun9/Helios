#pragma once

#include "Pch.hpp"

#include "Model.hpp"

#include "Graphics/API/Device.hpp"
#include "Graphics/API/PipelineState.hpp"
#include "Graphics/API/GraphicsContext.hpp"

namespace helios::scene
{
	struct SkyBoxCreationDesc
	{
		std::wstring equirectangularTexturePath{};
		DXGI_FORMAT format{};
		std::wstring name{};
	};

	// SkyBox abstraction handles creation of skybox (using equirectangular texture path) and create a cube map.
	// Also handles rendering of skybox, and the diffuse irradiance map.
	// Will also have the pipeline state for convertion of equirect texture to cubemap.
	// note(rtarun9) : Construct takes non const ref to device because of type qualifiers error.
	// needs to be checked.
	class SkyBox
	{
	public:
		SkyBox(gfx::Device* device, const SkyBoxCreationDesc& skyBoxCreationDesc);

		void Render(const gfx::GraphicsContext* graphicsContext, SkyBoxRenderResources& renderResources);

	private:
		static constexpr inline const wchar_t* SKYBOX_MODEL_PATH{ L"Assets/Models/Cube/glTF/Cube.gltf" };
		static constexpr uint32_t ENVIRONMENT_CUBEMAP_DIMENSION = 2048u;
		static constexpr uint32_t DIFFUSE_IRRADIANCE_MAP_TEXTURE_DIMENSION = 128u;

		std::unique_ptr<Model> mSkyBoxModel{};
		std::unique_ptr<gfx::PipelineState> mCubeMapFromEquirectPipelineState{};
		std::unique_ptr<gfx::PipelineState> mDiffuseIrradianceMapPipelineState{};

		std::unique_ptr<gfx::Texture> mSkyBoxTexture;
		std::unique_ptr<gfx::Texture> mDiffuseIrradianceMapTexture;
	};
}


