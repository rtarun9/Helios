#include "Pch.hpp"
#include "SandBox.hpp"

#include "BindlessRS.hlsli" 

using namespace helios;
using namespace DirectX;
using namespace DirectX::SimpleMath;

SandBox::SandBox(Config& config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	mDevice = std::make_unique<gfx::Device>();
}

void SandBox::OnUpdate()
{
}

void SandBox::OnRender()
{
	
	mDevice->BeginFrame();
	
	auto graphicsContext = mDevice->CreateGraphicsContext();
	auto currentBackBuffer = mDevice->GetCurrentBackBuffer();

	graphicsContext->AddResourceBarrier(currentBackBuffer->GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		
	graphicsContext->ClearRenderTargetView(currentBackBuffer, math::Color(0.0f, 1.0f, 0.0f, 0.0f));

	graphicsContext->AddResourceBarrier(currentBackBuffer->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	mDevice->EndFrame(std::move(graphicsContext));
	mDevice->Present();

	m_FrameIndex++;
}

void SandBox::OnDestroy()
{
}

void SandBox::OnKeyAction(uint8_t keycode, bool isKeyDown) 
{
	mCamera.HandleInput(keycode, isKeyDown);
}

void SandBox::OnResize() 
{
	if (m_Width != Application::GetClientWidth() || m_Height != Application::GetClientHeight())
	{
		mDevice->ResizeBuffers();
	
		m_Width = Application::GetClientWidth();
		m_Height = Application::GetClientHeight();
	}
}
