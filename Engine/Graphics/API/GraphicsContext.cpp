#include "Pch.hpp"

#include "GraphicsContext.hpp"
#include "Device.hpp"

namespace helios::gfx
{
	GraphicsContext::GraphicsContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
	{
		mCommandList = commandList;
	}

	void GraphicsContext::ResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState);
		mCommandList->ResourceBarrier(1u, &resourceBarrier);
	}

	void GraphicsContext::ClearRenderTargetView(BackBuffer* const backBuffer, const math::Color& color)
	{
		mCommandList->ClearRenderTargetView(backBuffer->backBufferDescriptorHandle.cpuDescriptorHandle, color, 0u, nullptr);
	}

	void GraphicsContext::SetDescriptorHeaps(Descriptor* const descriptor) const
	{
		std::array<ID3D12DescriptorHeap*, 1>  descriptorHeaps
		{
			descriptor->GetDescriptorHeap(),
		};


		mCommandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());
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

	void GraphicsContext::SetIndexBuffer(Buffer* const buffer)
	{
		D3D12_INDEX_BUFFER_VIEW indexBufferView
		{
			.BufferLocation = buffer->allocation->resource->GetGPUVirtualAddress(),
			.SizeInBytes = buffer->sizeInBytes,
			.Format = DXGI_FORMAT_R32_UINT,
		};

		mCommandList->IASetIndexBuffer(&indexBufferView);
	}

	void GraphicsContext::Set32BitGraphicsConstants(void* renderResources)
	{
		mCommandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, renderResources, 0u);
	}

	void GraphicsContext::SetDefaultViewportAndScissor(uint32_t width, uint32_t height) const
	{
		D3D12_VIEWPORT viewport
		{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(width),
			.Height = static_cast<float>(height),
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

	void GraphicsContext::SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
	{
		mCommandList->IASetPrimitiveTopology(primitiveTopology);
	}

	void GraphicsContext::SetRenderTarget(uint32_t count, BackBuffer* renderTarget)
	{
		mCommandList->OMSetRenderTargets(1u, &renderTarget->backBufferDescriptorHandle.cpuDescriptorHandle, FALSE, nullptr);
	}

	void GraphicsContext::DrawInstanceIndexed(uint32_t indicesCount)
	{
		mCommandList->DrawIndexedInstanced(indicesCount, 1u, 0u, 0u, 0u);
	}

	//void GraphicsContext::SetRenderTarget(RenderTarget* const renderTarget)
	//{
	//	auto rtIndexBufferView = RenderTarget::sIndexBuffer->GetBufferView();
	//
	//	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	mCommandList->IASetIndexBuffer(&rtIndexBufferView);
	//}
}

