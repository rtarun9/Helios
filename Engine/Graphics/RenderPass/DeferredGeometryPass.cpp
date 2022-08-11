#include "Pch.hpp"

#include "DeferredGeometryPass.hpp"

#include "../API/Device.hpp"

namespace helios::gfx
{
	DeferredGeometryPass::DeferredGeometryPass(const gfx::Device* device, const Uint2& dimensions)
	{
		// Create pipeline state.

		gfx::GraphicsPipelineStateCreationDesc deferredPassPipelineStateCreationDesc
		{
			.shaderModule
			{
				.vsShaderPath = L"Shaders/RenderPass/DeferredGeometryPassVS.cso",
				.psShaderPath = L"Shaders/RenderPass/DeferredGeometryPassPS.cso",
			},
			.rtvFormats = {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM},
			.rtvCount = 3u,
			.pipelineName = L"Deferred Geometry Pass Pipeline"
		};

		mDeferredPassPipelineState = std::make_unique<gfx::PipelineState>(device->GetDevice(), deferredPassPipelineStateCreationDesc);

		// Create MRT's for GPass.
		gfx::TextureCreationDesc positionRenderTargetTextureCreationDesc
		{
			.usage = gfx::TextureUsage::RenderTarget,
			.dimensions = dimensions,
			.format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.name = L"Deferred Pass Position Texture"
		};

		mDeferredPassRTs.positionRT = std::make_unique<gfx::RenderTarget>(device->CreateRenderTarget(positionRenderTargetTextureCreationDesc));

		gfx::TextureCreationDesc normalRenderTargetTextureCreationDesc
		{
			.usage = gfx::TextureUsage::RenderTarget,
			.dimensions = dimensions,
			.format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.name = L"Deferred Pass Normal Texture"
		};

		mDeferredPassRTs.normalRT = std::make_unique<gfx::RenderTarget>(device->CreateRenderTarget(normalRenderTargetTextureCreationDesc));

		gfx::TextureCreationDesc albedoRenderTargetTextureCreationDesc
		{
			.usage = gfx::TextureUsage::RenderTarget,
			.dimensions = dimensions,
			.format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.name = L"Deferred Pass Albedo Texture"
		};

		mDeferredPassRTs.albedoRT = std::make_unique<gfx::RenderTarget>(device->CreateRenderTarget(albedoRenderTargetTextureCreationDesc));
	}

	void DeferredGeometryPass::Render(scene::Scene* scene, gfx::GraphicsContext* graphicsContext, gfx::Texture* depthBuffer)
	{
		std::array<const gfx::RenderTarget*, 3u> renderTargets
		{
			mDeferredPassRTs.positionRT.get(),
			mDeferredPassRTs.normalRT.get(),
			mDeferredPassRTs.albedoRT.get(),
		};

		graphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		graphicsContext->ExecuteResourceBarriers();

		graphicsContext->SetGraphicsPipelineState(mDeferredPassPipelineState.get());
		graphicsContext->SetRenderTarget(renderTargets, depthBuffer);
		graphicsContext->SetDefaultViewportAndScissor();
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		graphicsContext->ClearRenderTargetView(renderTargets, std::array<float, 4u>{ 0.0f, 0.0f, 0.0f, 1.0f });
		graphicsContext->ClearDepthStencilView(depthBuffer, 1.0f);

		scene->RenderModels(graphicsContext);

		graphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		graphicsContext->ExecuteResourceBarriers();
	}

	void DeferredGeometryPass::Resize(gfx::Device* device, const Uint2& dimensions)
	{
		device->ResizeRenderTarget(mDeferredPassRTs.positionRT.get());
		device->ResizeRenderTarget(mDeferredPassRTs.normalRT.get());
		device->ResizeRenderTarget(mDeferredPassRTs.albedoRT.get());
	}
}
