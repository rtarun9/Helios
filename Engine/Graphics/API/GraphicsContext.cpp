#include "Pch.hpp"

#include "GraphicsContext.hpp"
#include "Device.hpp"

#include "Core/Application.hpp"

namespace helios::gfx
{
	GraphicsContext::GraphicsContext(Device& device) : mDevice(device)
	{
		mCommandList = device.GetGraphicsCommandQueue()->GetCommandList();

		// As all graphics context's require to set the descriptor heap before hand, the user has option to set them manually (for explicitness) or just let the constructor take care of this.
		SetDescriptorHeaps(mDevice.GetSrvCbvUavDescriptor());
	}

	void GraphicsContext::AddResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState) 
	{
		mResourceBarriers.push_back({ CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState) });
	}
	
	void GraphicsContext::AddResourceBarrier(std::span<const RenderTarget*> renderTargets, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState) 
	{
		for (const auto& rt : renderTargets)
		{
			mResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(rt->renderTexture->allocation->resource.Get(), previousState, newState));
		}
	}

	void GraphicsContext::ClearRenderTargetView(BackBuffer* const backBuffer, std::span<const float, 4> color)
	{
		mCommandList->ClearRenderTargetView(backBuffer->backBufferDescriptorHandle.cpuDescriptorHandle, color.data(), 0u, nullptr);
	}

	void GraphicsContext::ClearRenderTargetView(std::span<const RenderTarget*> renderTargets, std::span<const float, 4> color)
	{
		for (const auto& rt : renderTargets)
		{
			mCommandList->ClearRenderTargetView(mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(rt->renderTexture->rtvIndex).cpuDescriptorHandle, color.data(), 0u, nullptr);
		}
	}

	void GraphicsContext::ClearRenderTargetView(RenderTarget* renderTargets, std::span<const float, 4> color)
	{
			mCommandList->ClearRenderTargetView(mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(renderTargets->renderTexture->rtvIndex).cpuDescriptorHandle, color.data(), 0u, nullptr);
	}
	void GraphicsContext::ClearDepthStencilView(Texture* const depthStencilTexture, float depth)
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(depthStencilTexture->dsvIndex);

		mCommandList->ClearDepthStencilView(dsvDescriptorHandle.cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 1, 0u, nullptr);
	}

	void GraphicsContext::SetDescriptorHeaps(Descriptor* const descriptor) const
	{
		std::array<ID3D12DescriptorHeap*, 1>  descriptorHeaps
		{
			descriptor->GetDescriptorHeap(),
		};

		mCommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
	}

	void GraphicsContext::SetGraphicsPipelineState(PipelineState* pipelineState) const
	{
		mCommandList->SetGraphicsRootSignature(pipelineState->rootSignature.Get());
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

	// Specifies how the pipeline interprets vertex data bound to the input assember stage.
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
			DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(depthStencilTexture->dsvIndex);
			mCommandList->OMSetRenderTargets(1u, &renderTarget->backBufferDescriptorHandle.cpuDescriptorHandle, FALSE, &dsvDescriptorHandle.cpuDescriptorHandle);
		}
	}

	void GraphicsContext::SetRenderTarget(std::span<const RenderTarget*> renderTargets, const Texture* depthStencilTexture) const
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(depthStencilTexture->dsvIndex);

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles{};
		for (const auto& rt : renderTargets)
		{
			rtvHandles.emplace_back(mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(rt->renderTexture->rtvIndex).cpuDescriptorHandle);
		}

		mCommandList->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), rtvHandles.data(), FALSE, &dsvDescriptorHandle.cpuDescriptorHandle);
	}

	void GraphicsContext::SetRenderTarget(RenderTarget* renderTarget, const Texture* depthStencilTexture) const
	{
		DescriptorHandle dsvDescriptorHandle = mDevice.GetDsvDescriptor()->GetDescriptorHandleFromIndex(depthStencilTexture->dsvIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = mDevice.GetRtvDescriptor()->GetDescriptorHandleFromIndex(renderTarget->renderTexture->rtvIndex).cpuDescriptorHandle;
		mCommandList->OMSetRenderTargets(1u, &rtHandle, FALSE, &dsvDescriptorHandle.cpuDescriptorHandle);

	}
	void GraphicsContext::DrawInstanceIndexed(uint32_t indicesCount, uint32_t instanceCount) const
	{
		mCommandList->DrawIndexedInstanced(indicesCount, instanceCount, 0u, 0u, 0u);
	}
	 
	void GraphicsContext::CopyResource(ID3D12Resource* source, ID3D12Resource* destination)
	{
		mCommandList->CopyResource(destination, source);
	}

	void GraphicsContext::ExecuteResourceBarriers()
	{
		mCommandList->ResourceBarrier(static_cast<UINT>(mResourceBarriers.size()), mResourceBarriers.data());
		mResourceBarriers.clear();
	}
}

