#pragma once

#include "Helios.hpp"

// File with all constant buffer structs shared between C++ and HLSL.
#include "Common/ConstantBuffers.hlsli"

class SandBox : public helios::Engine
{
public:
	SandBox(helios::Config& config);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

	void OnKeyAction(uint8_t keycode, bool isKeyDown) override;
	void OnResize() override;

private:
	std::unique_ptr<helios::Model> mSciFiHelmet{};

	std::unique_ptr<helios::gfx::Buffer> mSceneBuffer{};
	SceneBuffer mSceneBufferData{};

	std::unique_ptr<helios::gfx::Texture> mDepthStencilTexture{};

	std::unique_ptr<helios::gfx::PipelineState> mPipelineState{};

	std::unique_ptr<helios::UIManager> mUIManager{};
};