#pragma once

#include "Pch.hpp"

#include "../../Scene/Scene.hpp"
#include "../API/PipelineState.hpp"

namespace helios::gfx
{
	// Bloom post processing inspired by the Call of duty Advanced Warfare presentation.
	// This class will take as input the rtv  and the bloom pass RT can be applied to the post processing shaders.
	// Unlike other renderpasses, it will create contexts on its own and execute them.
	// WARNING : Ensure all dependencies for BloomPass are executed before calling BloomPass::Render.
	class BloomPass
	{
	public:
		BloomPass(gfx::Device* device, const Uint2& dimensions);
		void OnResize(const gfx::Device* device, const Uint2& dimensions);

		void Render(gfx::Device* const device, gfx::RenderTarget* renderTarget, std::vector<std::unique_ptr<ComputeContext>>& computeContexts);

	public:
		static constexpr uint32_t BLOOM_MIP_LEVELS = 9u;
		static constexpr uint32_t BLOOM_PASSES = 1u;

		// For simplicity, the bloom pipeline state will be able to do a lot of various task based on the the bloom buffer values.
		std::unique_ptr<gfx::PipelineState> mBloomPipelineState{};
		
		std::unique_ptr<gfx::Texture> mPreFilterBloomTexture{};
		std::unique_ptr<gfx::Texture> mDownSampledBloomTextures{};
		std::unique_ptr<gfx::Texture> mUpSampledBloomTextures{};
		
		std::unique_ptr<gfx::Buffer> mBloomBuffer{};

		std::vector<uint32_t> mDownSamplingMipUavIndices{};
		std::vector<uint32_t> mUpSamplingMipUavIndices{};

		uint32_t mPreFilterSrvIndex{};
		uint32_t mPreFilterUavIndex{};

		BloomBuffer mBloomBufferData{};
	};
}