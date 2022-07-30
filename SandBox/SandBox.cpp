#include "Pch.hpp"
#include "SandBox.hpp"

#include "Common/BindlessRS.hlsli" 
#include "Common/ConstantBuffers.hlsli"

using namespace helios;
using namespace DirectX;

SandBox::SandBox(core::Config& config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	mDevice = std::make_unique<gfx::Device>();

	scene::ModelCreationDesc sciFiHelmetCreationDesc
	{
		.modelPath = L"Assets/Models/SciFiHelmet/glTF/SciFiHelmet.gltf",
		.modelName = L"SciFi Helmet",
	};

	mSciFiHelmet = std::make_unique<scene::Model>(mDevice.get(), sciFiHelmetCreationDesc);

	gfx::BufferCreationDesc sceneBufferCreationDesc
	{
		.usage = gfx::BufferUsage::ConstantBuffer,
		.name = L"Scene Buffer",
	};

	mSceneBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<SceneBuffer>(sceneBufferCreationDesc, std::span<SceneBuffer, 0u>{}));

	gfx::GraphicsPipelineStateCreationDesc graphicsPipelineStateCreationDesc
	{
		.vsShaderPath = L"Shaders/MeshViewer/MeshViewerVS.cso",
		.psShaderPath = L"Shaders/MeshViewer/MeshViewerPS.cso",
		.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
		.depthFormat = DXGI_FORMAT_D32_FLOAT,
		.pipelineName = L"Mesh Viewer Pipeline"
	};

	mPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(graphicsPipelineStateCreationDesc));
	
	gfx::GraphicsPipelineStateCreationDesc offscreenPipelineStateCreationDesc
	{
		.vsShaderPath = L"Shaders/OffscreenRTVS.cso",
		.psShaderPath = L"Shaders/OffscreenRTPS.cso",
		.rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT,
		.depthFormat = DXGI_FORMAT_D32_FLOAT,
		.pipelineName = L"Offscreen Render Target Pipeline"
	};

	mOffscreenPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(offscreenPipelineStateCreationDesc));
	
	gfx::TextureCreationDesc depthStencilTextureCreationDesc
	{
		.usage = gfx::TextureUsage::DepthStencil,
		.dimensions = mDimensions,
		.format = DXGI_FORMAT_R32_FLOAT,
		.name = L"Depth Stencil Texture"
	};

	mDepthStencilTexture = std::make_unique<gfx::Texture>(mDevice->CreateTexture(depthStencilTextureCreationDesc));

	gfx::TextureCreationDesc offscreenRenderTargetTextureCreationDesc
	{
		.usage = gfx::TextureUsage::RenderTarget,
		.dimensions = mDimensions,
		.format = DXGI_FORMAT_R16G16B16A16_FLOAT,
		.name = L"Offscreen Render Texture"
	};

	mOffscreenRT = std::make_unique<gfx::RenderTarget>(mDevice.get(), offscreenRenderTargetTextureCreationDesc);

	mUIManager = std::make_unique<ui::UIManager>(mDevice.get());

	mCamera = std::make_unique<scene::Camera>();
}

void SandBox::OnUpdate()
{
	mCamera->Update(static_cast<float>(core::Application::GetTimer().GetDeltaTime()));

	SceneBuffer sceneBufferData = 
	{
		.viewMatrix = mCamera->GetViewMatrix(),
		.projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), mAspectRatio, 0.1f, 1000.0f)
	};

	mSceneBuffer->Update(&sceneBufferData);

	mSciFiHelmet->GetTransform().Update();
}

void SandBox::OnRender()
{
	std::unique_ptr<gfx::GraphicsContext> graphicsContext = mDevice->GetGraphicsContext();
	gfx::BackBuffer* backBuffer = mDevice->GetCurrentBackBuffer();

	mDevice->BeginFrame();
	mUIManager->BeginFrame();

	// Configure offscreen render target's.
	std::array<const gfx::RenderTarget*, 1u> renderTargets
	{
		mOffscreenRT.get()
	};

	// RenderPass 1 : Draw the model's to the offscreen render target.
	{
		graphicsContext->ResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		graphicsContext->SetGraphicsPipelineState(mOffscreenPipelineState.get());
		graphicsContext->SetRenderTarget(renderTargets, mDepthStencilTexture.get());
		graphicsContext->SetDefaultViewportAndScissor();
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		static std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
		mUIManager->SetClearColor(clearColor);
		
		mSciFiHelmet->UpdateTransformUI(mUIManager.get());
		mCamera->UpdateUI(mUIManager.get());

		graphicsContext->ClearRenderTargetView(renderTargets, clearColor);
		graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

		SceneRenderResources sceneRenderResources
		{
			.sceneBufferIndex = mSceneBuffer->cbvIndex
		};

		mSciFiHelmet->Draw(graphicsContext.get(), sceneRenderResources);

		graphicsContext->ResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	
	// Render pass 2 : Render offscreen rt to swapchain's backbuffer and present.
	// The UI is directly drawn on swapchain's backbuffer rather than render target.
	{
		graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		graphicsContext->SetGraphicsPipelineState(mPipelineState.get());
		graphicsContext->SetRenderTarget(backBuffer, mDepthStencilTexture.get());
		graphicsContext->SetDefaultViewportAndScissor();
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		graphicsContext->ClearRenderTargetView(backBuffer, std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
		graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Draw function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = mOffscreenRT->renderTexture->srvIndex,
		};
		
		mOffscreenRT->Draw(graphicsContext.get(), rtvRenderResources);
		mUIManager->Render(graphicsContext.get());

		graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
	
	mDevice->EndFrame();

	mDevice->ExecuteContext(std::move(graphicsContext));
	mUIManager->EndFrame();

	mDevice->Present();

	mFrameIndex++;
}

void SandBox::OnDestroy()
{
}

void SandBox::OnKeyAction(uint8_t keycode, bool isKeyDown) 
{
	if (isKeyDown && keycode == VK_SPACE)
	{
		mUIManager->HideUI();
	}

	if (isKeyDown && keycode == VK_SHIFT)
	{
		mUIManager->ShowUI();
	}

	mCamera->HandleInput(keycode, isKeyDown);
}

void SandBox::OnResize() 
{
	if (mDimensions != core::Application::GetClientDimensions())
	{
		mDevice->ResizeBuffers();

		mDimensions = core::Application::GetClientDimensions();

		mUIManager->UpdateDisplaySize(core::Application::GetClientDimensions());

		mAspectRatio = static_cast<float>(mDimensions.x) / static_cast<float>(mDimensions.y);
	}
}
