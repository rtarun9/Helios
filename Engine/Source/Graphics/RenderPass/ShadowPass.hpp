#pragma once

#include "Pch.hpp"

#include "../API/Resources.hpp"

#include "Common/ConstantBuffers.hlsli"

namespace helios::gfx
{
	// Holds all data and renders scene from the lights perspective.
	class ShadowPass
	{
	public:
		ShadowPass(const gfx::Device* device);
		void Render(scene::Scene* scene, gfx::GraphicsContext* graphicsContext);
	
	public:
		static constexpr uint32_t SHADOW_MAP_DIMENSIONS = 2048u;

		std::unique_ptr<gfx::Texture> mDepthTexture{};
		std::unique_ptr<gfx::PipelineState> mShadowPipelineState{};
		std::unique_ptr<gfx::Buffer> mShadowMappingBuffer{};
		ShadowMappingBuffer mShadowMappingBufferData{};

	private:
		// note(rtarun9) : Undesirable to have this here, but need as in the first loop the current resource state of depth texture is DEPTH_WRITE, and not pixel shader resource view.
		// This flag will ignore execution of resource barrier for the first loop iteration.
		// Currently, there is no other alternative for this.
		bool mFirstLoopIteration{ true };
	};
}