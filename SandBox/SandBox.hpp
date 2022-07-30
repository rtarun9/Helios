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
	std::unique_ptr<helios::scene::Model> mSciFiHelmet{};

	std::unique_ptr<helios::scene::Camera> mCamera{};

	std::unique_ptr<helios::gfx::Buffer> sIndexBuffer;
	std::unique_ptr<helios::gfx::Buffer> sPositionBuffer;
	std::unique_ptr<helios::gfx::Buffer> sTextureCoordsBuffer;

	std::unique_ptr<helios::gfx::Buffer> mSceneBuffer{};

	std::unique_ptr<helios::gfx::Texture> mDepthStencilTexture{};
	std::unique_ptr<helios::gfx::RenderTarget> mOffscreenRT{};

	std::unique_ptr<helios::gfx::PipelineState> mOffscreenPipelineState{};
	std::unique_ptr<helios::gfx::PipelineState> mPipelineState{};

	std::unique_ptr<helios::ui::UIManager> mUIManager{};
};