#include "Pch.hpp"
#include "SandBox.hpp"

#include "Common/BindlessRS.hlsli" 
#include "Common/ConstantBuffers.hlsli"

using namespace helios;
using namespace DirectX;


SandBox::SandBox(Config& config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	mDevice = std::make_unique<gfx::Device>();

	ModelCreationDesc sciFiHelmetCreationDesc
	{
		.modelPath = L"Assets/Models/SciFiHelmet/glTF/SciFiHelmet.gltf",
		.modelName = L"SciFi Helmet",
	};

	mSciFiHelmet = std::make_unique<Model>(mDevice.get(), sciFiHelmetCreationDesc);

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
	
	gfx::TextureCreationDesc depthStencilTextureCreationDesc
	{
		.usage = gfx::TextureUsage::DepthStencil,
		.dimensions = mDimensions,
		.format = DXGI_FORMAT_R32_FLOAT,
		.name = L"Depth Stencil Texture"
	};

	mDepthStencilTexture = std::make_unique<gfx::Texture>(mDevice->CreateTexture(depthStencilTextureCreationDesc));

	mUIManager = std::make_unique<UIManager>(mDevice.get());

	mCamera = std::make_unique<Camera>();
}

void SandBox::OnUpdate()
{
	mCamera->Update(static_cast<float>(Application::GetTimer().GetDeltaTime()));

	SceneBuffer sceneBufferData = 
	{
		.viewMatrix = mCamera->GetViewMatrix(),
		.projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), mAspectRatio, 0.1f, 100.0f)
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
	mUIManager->Begin(L"Menu");

	graphicsContext->SetGraphicsPipelineState(mPipelineState.get());

	graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	mSciFiHelmet->UpdateTransformUI(mUIManager.get());
	mCamera->UpdateUI(mUIManager.get());

	static std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	mUIManager->SetClearColor(clearColor);
	graphicsContext->ClearRenderTargetView(backBuffer, clearColor);
	graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

	graphicsContext->SetRenderTarget(1u, backBuffer, mDepthStencilTexture.get());
	graphicsContext->SetDefaultViewportAndScissor();
	graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	SceneRenderResources sceneRenderResources
	{
		.sceneBufferIndex = mSceneBuffer->srvCbvUavIndex
	};

	mSciFiHelmet->Draw(graphicsContext.get(), sceneRenderResources);

	mUIManager->End();
	mUIManager->EndFrame(graphicsContext.get());
	graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	mDevice->EndFrame();

	mDevice->ExecuteContext(std::move(graphicsContext));

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
	if (mDimensions != Application::GetClientDimensions())
	{
		mDevice->ResizeBuffers();

		mDimensions = Application::GetClientDimensions();

		mUIManager->UpdateDisplaySize(Application::GetClientDimensions());

		mAspectRatio = static_cast<float>(mDimensions.x) / static_cast<float>(mDimensions.y);
	}
}
