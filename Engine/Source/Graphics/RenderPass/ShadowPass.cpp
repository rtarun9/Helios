#include "ShadowPass.hpp"

#include "Scene/Light.hpp"

using namespace DirectX;

namespace helios::gfx
{
	ShadowPass::ShadowPass(const gfx::Device* device)
	{
		// Create depth texture.
		gfx::TextureCreationDesc depthTextureCreationDesc
		{
			.usage = gfx::TextureUsage::DepthStencil,
			.dimensions = {SHADOW_MAP_DIMENSIONS, SHADOW_MAP_DIMENSIONS},
			.format = DXGI_FORMAT_D32_FLOAT,
			.mipLevels = 1u,
			.depthOrArraySize = 1u,
			.name = L"Shadow Mapping Depth Texture"
		};

		mDepthTexture = std::make_unique<gfx::Texture>(device->CreateTexture(depthTextureCreationDesc));

		// Create pipeline state.
		gfx::GraphicsPipelineStateCreationDesc depthPipelineStateCreationDesc
		{
			.shaderModule
            {
                .vsShaderPath = (L"Shaders/RenderPass/PCFShadowMappingVS.cso"),
                .psShaderPath = (L"Shaders/RenderPass/PCFShadowMappingPS.cso"),
            },
            .rtvFormats = {},
            .rtvCount = 0,
			.cullMode = D3D12_CULL_MODE_FRONT,
            .pipelineName = L"Shadow Mapping Pipeline State",
		};

		mShadowPipelineState = std::make_unique<gfx::PipelineState>(device->CreatePipelineState(depthPipelineStateCreationDesc));

		// Create shadow mapping constant buffer.
		gfx::BufferCreationDesc shadowMappingBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Shadow Mapping Buffer"
		};

		mShadowMappingBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<ShadowMappingBuffer>(shadowMappingBufferCreationDesc));

		// Setup initial data of shadow mapping buffer.
		mShadowMappingBufferData =
		{
			.backOffDistance = 200.0f,
			.extents = 99.0f,
			.nearPlane = 1.0f,
			.farPlane = 480.0f
		};
	}

	void ShadowPass::Render(scene::Scene* scene, gfx::GraphicsContext* graphicsContext)
	{
		// Setup and update shadow constant buffer.
		static D3D12_VIEWPORT viewport
		{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(SHADOW_MAP_DIMENSIONS),
			.Height = static_cast<float>(SHADOW_MAP_DIMENSIONS),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};

		auto directionalLightPosition = scene::Light::GetLightBufferData()->lightPosition[DIRECTIONAL_LIGHT_OFFSET];
		auto directionalLightPositionVector = DirectX::XMLoadFloat4(&directionalLightPosition);

		DirectX::XMVECTOR lightPosition = DirectX::XMVectorZero() - mShadowMappingBufferData.backOffDistance * DirectX::XMVector4Normalize(directionalLightPositionVector);

		DirectX::XMMATRIX lightViewMatrix = DirectX::XMMatrixLookAtLH(lightPosition, DirectX::XMVectorZero(), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		DirectX::XMMATRIX lightProjectionMatrix = DirectX::XMMatrixOrthographicOffCenterLH(-mShadowMappingBufferData.extents, mShadowMappingBufferData.extents, -mShadowMappingBufferData.extents, mShadowMappingBufferData.extents, mShadowMappingBufferData.nearPlane,mShadowMappingBufferData.farPlane);
	
		mShadowMappingBufferData.viewProjectionMatrix = lightViewMatrix * lightProjectionMatrix;
		mShadowMappingBuffer->Update(&mShadowMappingBufferData);

		if (!mFirstLoopIteration)
		{
			graphicsContext->AddResourceBarrier(mDepthTexture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			graphicsContext->ExecuteResourceBarriers();
		}


		graphicsContext->SetGraphicsPipelineState(mShadowPipelineState.get());
		graphicsContext->SetRenderTarget(mDepthTexture.get());
		graphicsContext->SetViewportAndScissor(viewport);
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		graphicsContext->ClearDepthStencilView(mDepthTexture.get(), 1.0f);

		ShadowMappingRenderResources renderResources
		{
			.shadowMappingBufferIndex = gfx::Buffer::GetCbvIndex(mShadowMappingBuffer.get()),
		};

		scene->RenderModels(graphicsContext, renderResources);

		graphicsContext->AddResourceBarrier(mDepthTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		graphicsContext->ExecuteResourceBarriers();

		mFirstLoopIteration = false;
	}
}