#include "Pch.hpp"
#include "SandBox.hpp"

#include "Common/BindlessRS.hlsli" 

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

	std::array<SimpleMath::Vector3, 3> testData
	{
		Vector3{1.0f, -1.0f, 0.0f},
		Vector3{-1.0f, -1.0f, 0.0f},
		Vector3{0.0f, 1.0f, 0.0f}
	};

	gfx::BufferCreationDesc testBufferCreationDesc
	{
		.usage = gfx::BufferUsage::StructuredBuffer,
		.size = testData.size(),
		.stride = sizeof(SimpleMath::Vector3),
		.name = L"Test Vertex Buffer",
	};

	mPositionBuffer  = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(testBufferCreationDesc, testData.data()));

	std::array<SimpleMath::Vector3, 3> testColorData
	{
		Vector3{1.0f, 0.0f, 0.0f},
		Vector3{0.0f, 1.0f, 0.0f},
		Vector3{0.0f, 0.0f, 1.0f}
	};

	gfx::BufferCreationDesc testBufferCreationDescC
	{
		.usage = gfx::BufferUsage::StructuredBuffer,
		.size = testColorData.size(),
		.stride = sizeof(SimpleMath::Vector3),
		.name = L"Test Color Buffer",
	};


	mColorBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(testBufferCreationDescC, testColorData.data()));

	std::array<uint32_t, 3> testIndices
	{
		0u, 1u, 2u
	};

	gfx::BufferCreationDesc testBufferCreationDescD
	{
		.usage = gfx::BufferUsage::IndexBuffer,
		.size = testIndices.size(),
		.stride = sizeof(uint32_t),
		.name = L"Test Index Buffer",
	};

	mIndexBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer(testBufferCreationDescD, testIndices.data()));

	gfx::GraphicsPipelineStateCreationDesc graphicsPipelineStateCreationDesc
	{
		.vsShaderPath = L"Shaders/MeshViewer/MeshViewerVS.cso",
		.psShaderPath = L"Shaders/MeshViewer/MeshViewerPS.cso",
		.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
		.depthFormat = DXGI_FORMAT_UNKNOWN,
		.pipelineName = L"Mesh Viewer Pipeline"
	};

	mPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(graphicsPipelineStateCreationDesc));
	
}

void SandBox::OnUpdate()
{
}

void SandBox::OnRender()
{
	std::unique_ptr<gfx::GraphicsContext> graphicsContext = mDevice->GetGraphicsContext();
	gfx::BackBuffer* backBuffer = mDevice->GetCurrentBackBuffer();

	mDevice->BeginFrame();

	graphicsContext->SetDescriptorHeaps(mDevice->GetSrvCbvUavDescriptor());

	graphicsContext->SetGraphicsPipelineState(mPipelineState.get());

	graphicsContext->ResourceBarrier(backBuffer->backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	graphicsContext->ClearRenderTargetView(backBuffer, SimpleMath::Color(0.1f, 0.1f, 0.1f, 1.0f));
	graphicsContext->SetRenderTarget(1u, backBuffer);
	graphicsContext->SetDefaultViewportAndScissor(mWidth, mHeight);
	graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	graphicsContext->SetIndexBuffer(mIndexBuffer.get());

	MeshViewerRenderResources meshViewerRenderResources
	{
		.positionBufferIndex = mPositionBuffer->srvUavCbvIndexInDescriptorHeap,
		.colorBufferIndex = mColorBuffer->srvUavCbvIndexInDescriptorHeap
	};

	graphicsContext->Set32BitGraphicsConstants(&meshViewerRenderResources);


	graphicsContext->DrawInstanceIndexed(3u);

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
	if (mWidth != Application::GetClientWidth() || mHeight != Application::GetClientHeight())
	{
		mDevice->ResizeBuffers();
	
		mWidth = Application::GetClientWidth();
		mHeight = Application::GetClientHeight();
	}
}
