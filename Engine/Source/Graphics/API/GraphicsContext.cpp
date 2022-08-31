

#include "GraphicsContext.hpp"
#include "Device.hpp"
#include "Descriptor.hpp"

#include "Core/Application.hpp"

namespace helios::gfx
{
	GraphicsContext::GraphicsContext(Device* device, const gfx::PipelineState* pipelineState) : mDevice(*device)
	{
		mCommandList = device->GetGraphicsCommandQueue()->GetCommandList(pipelineState);

		// As all graphics context's require to set the descriptor heap before hand, the user has option to set them manually (for explicitness) or just let the constructor take care of this.
		std::array<gfx::Descriptor*, 2> descriptors
		{
			mDevice.GetSrvCbvUavDescriptor(),
			mDevice.GetSamplerDescriptor()
		};

		SetDescriptorHeaps(descriptors);

		mCommandList->SetGraphicsRootSignature(PipelineState::rootSignature.Get());
	}

	void GraphicsContext::ClearRenderTargetView(BackBuffer* const backBuffer, std::span<const float, 4> color)
	{
		mCommandList->ClearRenderTargetView(backBuffer->backBufferDescriptorHandle.cpuDescriptorHandle, color.data(), 0u, nullptr);
	}

	void GraphicsContext::ClearRenderTargetView(std::span<const RenderTarget*> renderTargets, std::span<const float, 4> color)
	{
		for (const auto& rt : renderTargets)
		{
			mCommandList->ClearRenderTargetView(mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(RenderTarget::GetRenderTextureRTVIndex(rt)).cpuDescriptorHandle, color.data(), 0u, nullptr);
		}
	}

	void GraphicsContext::ClearRenderTargetView(RenderTarget* renderTargets, std::span<const float, 4> color)
	{
			mCommandList->ClearRenderTargetView(mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(RenderTarget::GetRenderTextureRTVIndex(renderTargets)).cpuDescriptorHandle, color.data(), 0u, nullptr);
	}
	void GraphicsContext::ClearDepthStencilView(Texture* const depthStencilTexture, float depth)
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(gfx::Texture::GetDsvIndex((depthStencilTexture)));

		mCommandList->ClearDepthStencilView(dsvDescriptorHandle.cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 1, 0u, nullptr);
	}

	void GraphicsContext::SetDescriptorHeaps(std::array<gfx::Descriptor*, 2> descriptors) const
	{
		std::array<ID3D12DescriptorHeap*, 2>  descriptorHeaps
		{
			descriptors[0]->GetDescriptorHeap(),
			descriptors[1]->GetDescriptorHeap(),
		};

		mCommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
	}

	void GraphicsContext::SetGraphicsPipelineState(PipelineState* pipelineState) const
	{
		mCommandList->SetPipelineState(pipelineState->pipelineStateObject.Get());
	}

	void GraphicsContext::SetComputePipelineState(PipelineState* pipelineState) const
	{
		mCommandList->SetComputeRootSignature(pipelineState->rootSignature.Get());
		mCommandList->SetPipelineState(pipelineState->pipelineStateObject.Get());
	}

	void GraphicsContext::SetGraphicsRootSignature(PipelineState* pipelineState) const
	{
		mCommandList->SetGraphicsRootSignature(pipelineState->rootSignature.Get());
	}

	void GraphicsContext::SetComputeRootSignature(PipelineState* pipelineState) const
	{
		mCommandList->SetComputeRootSignature(pipelineState->rootSignature.Get());
	}

	void GraphicsContext::SetPipelineStateObject(PipelineState* pipelineState) const
	{
		mCommandList->SetPipelineState(pipelineState->pipelineStateObject.Get());
	}

	void GraphicsContext::SetIndexBuffer(Buffer* const buffer) const
	{
		D3D12_INDEX_BUFFER_VIEW indexBufferView
		{
			.BufferLocation = buffer->allocation->resource->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(buffer->sizeInBytes),
			.Format = DXGI_FORMAT_R32_UINT,
		};

		mCommandList->IASetIndexBuffer(&indexBufferView);
	}

	void GraphicsContext::Set32BitGraphicsConstants(const void* renderResources) const
	{
		mCommandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, renderResources, 0u);
	}

	void GraphicsContext::SetDefaultViewportAndScissor() const
	{
		D3D12_VIEWPORT viewport
		{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(core::Application::GetClientDimensions().x),
			.Height = static_cast<float>(core::Application::GetClientDimensions().y),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};

		static D3D12_RECT scissorRect
		{
			.left = 0u,
			.top = 0u,
			.right = LONG_MAX,
			.bottom = LONG_MAX
		};

		mCommandList->RSSetViewports(1u, &viewport);
		mCommandList->RSSetScissorRects(1u, &scissorRect);
	}

	void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT& viewport) const
	{
		static D3D12_RECT scissorRect
		{
			.left = 0u,
			.top = 0u,
			.right = LONG_MAX,
			.bottom = LONG_MAX
		};

		mCommandList->RSSetViewports(1u, &viewport);
		mCommandList->RSSetScissorRects(1u, &scissorRect);
	}

	// Specifies how the pipeline interprets vertex data bound to the input assembler stage.
	// i.e if topology type is POINTLIST, vertex data is interpreted as list of points.
	void GraphicsContext::SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY primitiveTopology) const
	{
		mCommandList->IASetPrimitiveTopology(primitiveTopology);
	}

	void GraphicsContext::SetRenderTarget(BackBuffer* renderTarget, const Texture* depthStencilTexture) const
	{
		
		if (!depthStencilTexture)
		{
			mCommandList->OMSetRenderTargets(1u, &renderTarget->backBufferDescriptorHandle.cpuDescriptorHandle, FALSE, nullptr);
		}
		else
		{
			DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(Texture::GetDsvIndex(depthStencilTexture));
			mCommandList->OMSetRenderTargets(1u, &renderTarget->backBufferDescriptorHandle.cpuDescriptorHandle, FALSE, &dsvDescriptorHandle.cpuDescriptorHandle);
		}
	}

	void GraphicsContext::SetRenderTarget(std::span<const RenderTarget*> renderTargets, const Texture* depthStencilTexture) const
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(Texture::GetDsvIndex(depthStencilTexture));

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles{};
		for (const auto& rt : renderTargets)
		{
			rtvHandles.emplace_back(mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(RenderTarget::GetRenderTextureRTVIndex(rt)).cpuDescriptorHandle);
		}

		mCommandList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), rtvHandles.data(), TRUE, &dsvDescriptorHandle.cpuDescriptorHandle);
	}

	void GraphicsContext::SetRenderTarget(RenderTarget* renderTarget, const Texture* depthStencilTexture) const
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(Texture::GetDsvIndex(depthStencilTexture));
		D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(RenderTarget::GetRenderTextureRTVIndex(renderTarget)).cpuDescriptorHandle;
		mCommandList->OMSetRenderTargets(1u, &rtHandle, FALSE, &dsvDescriptorHandle.cpuDescriptorHandle);
	}

	void GraphicsContext::SetRenderTarget(const Texture* depthStencilTexture) const
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(Texture::GetDsvIndex(depthStencilTexture));
		mCommandList->OMSetRenderTargets(0u, nullptr, FALSE, &dsvDescriptorHandle.cpuDescriptorHandle);
	}

	void GraphicsContext::DrawInstanceIndexed(uint32_t indicesCount, uint32_t instanceCount) const
	{
		mCommandList->DrawIndexedInstanced(indicesCount, instanceCount, 0u, 0u, 0u);
	}

	void GraphicsContext::DrawIndexed(uint32_t indicesCount, uint32_t instanceCount) const
	{
		mCommandList->DrawInstanced(indicesCount, instanceCount, 0u, 0u);
	}
	 
	void GraphicsContext::CopyResource(ID3D12Resource* source, ID3D12Resource* destination)
	{
		mCommandList->CopyResource(destination, source);
	}
}

