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
	};
}