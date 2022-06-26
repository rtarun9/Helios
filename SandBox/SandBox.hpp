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
	// TEST
	std::unique_ptr<helios::gfx::Buffer> mPositionBuffer{};
	std::unique_ptr<helios::gfx::Buffer> mColorBuffer{};
	std::unique_ptr<helios::gfx::Buffer> mIndexBuffer{};
	std::unique_ptr<helios::gfx::PipelineState> mPipelineState{};
};