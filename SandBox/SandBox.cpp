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
		.numComponenets = cubePositions.size(),
		.stride = sizeof(math::XMFLOAT3),
		.name = L"Cube Positions Buffer",
	};

	mPositionBuffer  = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(cubePositionBufferCreationDesc, cubePositions.data()));

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
		.numComponenets = cubeColors.size(),
		.stride = sizeof(math::XMFLOAT3),
		.name = L"Cube Color Buffer",
	};

	mColorBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(cubeColorBufferCreationDesc, cubeColors.data()));
	
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
		.numComponenets = cubeIndices.size(),
		.stride = sizeof(uint32_t),
		.name = L"Test Index Buffer",
	};

	mIndexBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(cubeIndicesBufferCreationDesc, cubeIndices.data()));

	gfx::BufferCreationDesc transformBufferCreationDesc
	{
		.usage = gfx::BufferUsage::ConstantBuffer,
		.numComponenets = 1u,
		.stride = sizeof(TransformData),
		.name = L"Transform Data Buffer",
	};

	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);

	mTransformMatrix = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(transformBufferCreationDesc));

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
}

void SandBox::OnUpdate()
{
	const XMVECTOR eyePosition = XMVectorSet(0.0, 0.0, -10.0, 1.0);
	const XMVECTOR focusPoint = XMVectorSet(0.0, 0.0, 0.0, 1.0);
	const XMVECTOR upDirection = XMVectorSet(0.0, 1.0, 0.0, 0.0);

	TransformMatrix transfomationMatrix
	{
		.modelMatrix = math::XMMatrixRotationY(Application::GetTimer().GetTotalTime() / 10.0f),
		.viewMatrix = math::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection),
		.projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), mAspectRatio, 0.1f, 100.0f)
	};

	mTransformMatrix->Update(&transfomationMatrix, sizeof(TransformMatrix));
}

void SandBox::OnRender()
{
	std::unique_ptr<gfx::GraphicsContext> graphicsContext = mDevice->GetGraphicsContext();
	gfx::BackBuffer* backBuffer = mDevice->GetCurrentBackBuffer();

	mDevice->BeginFrame();

	graphicsContext->SetDescriptorHeaps(mDevice->GetSrvCbvUavDescriptor());

	graphicsContext->SetGraphicsPipelineState(mPipelineState.get());

	graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	graphicsContext->ClearRenderTargetView(backBuffer, std::array{ 0.1f, 0.1f, 0.1f, 1.0f });
	graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

	graphicsContext->SetRenderTarget(1u, backBuffer, mDepthStencilTexture.get());
	graphicsContext->SetDefaultViewportAndScissor(mDimensions);
	graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	graphicsContext->SetIndexBuffer(mIndexBuffer.get());

	MeshViewerRenderResources meshViewerRenderResources
	{
		.positionBufferIndex = mPositionBuffer->srvCbvIndex,
		.colorBufferIndex = mColorBuffer->srvCbvIndex,
		.cameraDataBufferIndex = mTransformMatrix->srvCbvIndex
	};

	graphicsContext->Set32BitGraphicsConstants(&meshViewerRenderResources);

	graphicsContext->DrawInstanceIndexed(36u);

	graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	mDevice->ExecuteContext(std::move(graphicsContext));

	mDevice->EndFrame();
	mDevice->Present();

	mFrameIndex++;
}

void SandBox::OnDestroy()
{
}

void SandBox::OnKeyAction(uint8_t keycode, bool isKeyDown) 
{
}

void SandBox::OnResize() 
{
	if (mDimensions != Application::GetClientDimensions())
	{
		mDevice->ResizeBuffers();
	
		mDimensions = Application::GetClientDimensions();
	}
}
