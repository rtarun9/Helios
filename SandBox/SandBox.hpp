#pragma once

#include "Helios.hpp"

// File with all constant buffer structs shared between C++ and HLSL.
#include "ConstantBuffers.hlsli"

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

private:
	D3D12_VIEWPORT mViewport{};
	D3D12_RECT mScissorRect{ .left = 0, .top = 0, .right = LONG_MAX, .bottom = LONG_MAX };

	helios::Camera mCamera{};
};