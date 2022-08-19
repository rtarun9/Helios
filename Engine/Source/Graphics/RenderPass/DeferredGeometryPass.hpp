#pragma once



#include "../API/Resources.hpp"
#include "../API/PipelineState.hpp"

#include "../../Scene/Scene.hpp"

namespace helios::gfx
{
	// The different RT's the GPass shader can output too.

	// For reference : 
	// float4 albedo : SV_Target0;
	// float4 positionEmissive : SV_Target1;
	// float4 normalEmissive : SV_Target2;
	// float4 aoMetalRoughnessEmissive : SV_Target3;
	struct DeferredPassRTs
	{
		std::unique_ptr<gfx::RenderTarget> albedoRT{};
		std::unique_ptr<gfx::RenderTarget> positionEmissiveRT{};
		std::unique_ptr<gfx::RenderTarget> normalEmissiveRT{};
		std::unique_ptr<gfx::RenderTarget> aoMetalRoughnessEmissiveRT{};
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
