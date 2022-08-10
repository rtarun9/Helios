#pragma once

#include "Pch.hpp"

#include "../API/Resources.hpp"
#include "../API/PipelineState.hpp"

#include "../../Scene/Scene.hpp"

namespace helios::gfx
{
	// The different RT's the GPass shader can output too.
	struct DeferredPassRTs
	{
		std::unique_ptr<gfx::RenderTarget> positionRT{};
		std::unique_ptr<gfx::RenderTarget> albedoRT{};
		std::unique_ptr<gfx::RenderTarget> normalRT{};
	};

	// This abstraction produces MRT's for various attributes (positions, albedo, normal etc) for a given scene. Handles resizing of all RT's too.
	class DeferredGeometryPass
	{
	public:
		DeferredGeometryPass(const gfx::Device* device, const Uint2& dimensions);

		void Render(scene::Scene* scene, gfx::GraphicsContext* graphicsContext, gfx::Texture* depthBuffer);

		void Resize(gfx::Device* device, const Uint2& dimensions);

	public:
		DeferredPassRTs mDeferredPassRTs{};

		std::unique_ptr<gfx::PipelineState> mDeferredPassPipelineState{};
	};

}
