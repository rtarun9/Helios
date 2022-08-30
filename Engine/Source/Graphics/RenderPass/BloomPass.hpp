#pragma once

#include "Pch.hpp"

#include "../../Scene/Scene.hpp"
#include "../API/PipelineState.hpp"

namespace helios::gfx
{
	// Bloom post processing inspired by the Call of duty Advanced Warfare presentation.
	// This class will take as input the rtv  and the bloom pass RT can be applied to the post processing shaders.
	class BloomPass
	{
	public:
		BloomPass(const gfx::Device* device, const Uint2& dimensions);
		void OnResize(const gfx::Device* device, const Uint2& dimensions);

		void Render(gfx::Device* const device, gfx::RenderTarget* renderTarget, gfx::ComputeContext* computeContext);

	public:
		// For simplicity, the bloom pipeline state will be able to lot of various task based on the the bloom buffer values.
		std::unique_ptr<gfx::PipelineState> mBloomPipelineState{};
		std::unique_ptr<gfx::Texture> mBloomTextures{};
		std::unique_ptr<gfx::Buffer> mBloomBuffer{};

		std::unique_ptr<gfx::Buffer> mMipMapBuffer{};
		std::vector<uint32_t> mMipUavIndices{};
		uint32_t mSrvIndex{};

		BloomBuffer mBloomBufferData{};
	};
}