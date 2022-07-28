#include "Pch.hpp"
#include "SandBox.hpp"

#include "Common/BindlessRS.hlsli" 

using namespace helios;
using namespace DirectX;

struct alignas(256) TransformMatrix 
{
	math::XMMATRIX modelMatrix{ math::XMMatrixIdentity()};
	math::XMMATRIX viewMatrix{math::XMMatrixIdentity()};
	math::XMMATRIX projectionMatrix{math::XMMatrixIdentity()};
};

struct Vertex
{
	
};

SandBox::SandBox(Config& config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	mDevice = std::make_unique<gfx::Device>();

	std::array<math::XMFLOAT3, 8> cubePositions
	{
		XMFLOAT3(-1.0f, -1.0f, -1.0f), 
		XMFLOAT3(-1.0f,  1.0f, -1.0f),
		XMFLOAT3(1.0f,  1.0f, -1.0f),	
		XMFLOAT3(1.0f, -1.0f, -1.0f),	
		XMFLOAT3(-1.0f, -1.0f,  1.0f),
		XMFLOAT3(-1.0f,  1.0f,  1.0f),
		XMFLOAT3(1.0f,  1.0f,  1.0f),	
		XMFLOAT3(1.0f, -1.0f,  1.0f),	
	};

	gfx::BufferCreationDesc cubePositionBufferCreationDesc
	{
		.usage = gfx::BufferUsage::StructuredBuffer,
		.name = L"Cube Positions Buffer",
	};

	mPositionBuffer  = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<math::XMFLOAT3>(cubePositionBufferCreationDesc, cubePositions));

	std::array<math::XMFLOAT3, 8> cubeColors
	{
		XMFLOAT3(0.0f, 0.0f, 0.0f), 
		XMFLOAT3(0.0f, 1.0f, 0.0f), 
		XMFLOAT3(1.0f, 1.0f, 0.0f), 
		XMFLOAT3(1.0f, 0.0f, 0.0f), 
		XMFLOAT3(0.0f, 0.0f, 1.0f), 
		XMFLOAT3(0.0f, 1.0f, 1.0f), 
		XMFLOAT3(1.0f, 1.0f, 1.0f), 
		XMFLOAT3(1.0f, 0.0f, 1.0f)  
	};

	gfx::BufferCreationDesc cubeColorBufferCreationDesc
	{
		.usage = gfx::BufferUsage::StructuredBuffer,
		.name = L"Cube Color Buffer",
	};

	mColorBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<math::XMFLOAT3>(cubeColorBufferCreationDesc, cubeColors));
	
	std::array<uint32_t, 36> cubeIndices
	{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	gfx::BufferCreationDesc cubeIndicesBufferCreationDesc
	{
		.usage = gfx::BufferUsage::IndexBuffer,
		.name = L"Test Index Buffer",
	};

	mIndexBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<uint32_t>(cubeIndicesBufferCreationDesc, cubeIndices));

	gfx::BufferCreationDesc transformBufferCreationDesc
	{
		.usage = gfx::BufferUsage::ConstantBuffer,
		.name = L"Transform Data Buffer",
	};

	mTransformMatrix = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<TransformBuffer>(transformBufferCreationDesc, std::span<TransformBuffer, 0u>{}));

	gfx::TextureCreationDesc testTextureCreationDesc
	{
		.usage = gfx::TextureUsage::TextureFromPath,
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.name = L"Test Texture",
		.path = L"Assets/Textures/TestTexture.png",
	};

	mTestTexture = std::make_unique<gfx::Texture>(mDevice->CreateTexture(testTextureCreationDesc));

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
}

void SandBox::OnUpdate()
{
	static const XMVECTOR eyePosition = XMVectorSet(0.0, 0.0, -10.0, 1.0);
	static const XMVECTOR focusPoint = XMVectorSet(0.0, 0.0, 0.0, 1.0);
	static const XMVECTOR upDirection = XMVectorSet(0.0, 1.0, 0.0, 0.0);

	TransformMatrix transfomationMatrix
	{
		.modelMatrix = math::XMMatrixRotationY((float)Application::GetTimer().GetTotalTime() / 10.0f),
		.viewMatrix = math::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection),
		.projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), mAspectRatio, 0.1f, 100.0f)
	};

	mTransformMatrix->Update(&transfomationMatrix);
}

void SandBox::OnRender()
{
	std::unique_ptr<gfx::GraphicsContext> graphicsContext = mDevice->GetGraphicsContext();
	gfx::BackBuffer* backBuffer = mDevice->GetCurrentBackBuffer();

	mDevice->BeginFrame();

	mUIManager->BeginFrame();
	graphicsContext->SetGraphicsPipelineState(mPipelineState.get());

	graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	static std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	mUIManager->SetClearColor(clearColor);
	graphicsContext->ClearRenderTargetView(backBuffer, clearColor);
	graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

	graphicsContext->SetRenderTarget(1u, backBuffer, mDepthStencilTexture.get());
	graphicsContext->SetDefaultViewportAndScissor();
	graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	graphicsContext->SetIndexBuffer(mIndexBuffer.get());

	MeshViewerRenderResources meshViewerRenderResources
	{
		.positionBufferIndex = mPositionBuffer->srbCbvUavIndex,
		.colorBufferIndex = mColorBuffer->srbCbvUavIndex,
		.cameraDataBufferIndex = mTransformMatrix->srbCbvUavIndex
	};

	graphicsContext->Set32BitGraphicsConstants(&meshViewerRenderResources);

	graphicsContext->DrawInstanceIndexed(36u);

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
}

void SandBox::OnResize() 
{
	if (mDimensions != Application::GetClientDimensions())
	{
		mDevice->ResizeBuffers();
	
		mDimensions = Application::GetClientDimensions();

		mUIManager->UpdateDisplaySize(Application::GetClientDimensions());
	}
}
