#pragma once

#include "Pch.hpp"

#include "../API/Resources.hpp"
#include "../API/PipelineState.hpp"

#include "../../Scene/Scene.hpp"

namespace helios::gfx
{
	struct DeferredPassRTs
	{
		std::unique_ptr<gfx::RenderTarget> positionRT{};
		std::unique_ptr<gfx::RenderTarget> albedoRT{};
		std::unique_ptr<gfx::RenderTarget> normalRT{};
	};

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
