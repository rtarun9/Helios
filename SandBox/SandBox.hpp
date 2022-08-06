#pragma once

#include "Helios.hpp"

// File with all constant buffer structs shared between C++ and HLSL.
#include "Common/ConstantBuffers.hlsli"

class SandBox : public helios::core::Engine
{
public:
	SandBox(helios::core::Config& config);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

	void OnKeyAction(uint8_t keycode, bool isKeyDown) override;
	void OnResize() override;

private:
	std::unique_ptr<helios::scene::Scene> mScene{};

	std::unique_ptr<helios::gfx::Buffer> mPostProcessBuffer{};
	PostProcessBuffer mPostProcessBufferData{};

	std::unique_ptr<helios::gfx::Texture> mDepthStencilTexture{};
	
	// All post processing effects are processed and stored in here (final result of the main scene).
	std::unique_ptr<helios::gfx::RenderTarget> mPostProcessingRT{};
	std::unique_ptr<helios::gfx::RenderTarget> mOffscreenRT{};
	// Contains the final image that is to be rendered to the swapchain.
	std::unique_ptr<helios::gfx::RenderTarget> mFinalRT{};

	std::unique_ptr<helios::gfx::PipelineState> mPBRPipelineState{};
	std::unique_ptr<helios::gfx::PipelineState> mLightPipelineState{};
	std::unique_ptr<helios::gfx::PipelineState> mFinalPipelineState{};
	std::unique_ptr<helios::gfx::PipelineState> mPostProcessingStaet{};
	std::unique_ptr<helios::gfx::PipelineState> mSkyBoxPipelineState{};

	std::unique_ptr<helios::editor::Editor> mEditor{};
};