#include "Pch.hpp"

#include "GraphicsContext.hpp"

namespace helios::gfx
{
	GraphicsContext::GraphicsContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
	{
		mCommandList = commandList;
	}

	void GraphicsContext::AddResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState);
		mCommandList->ResourceBarrier(1u, &resourceBarrier);
	}

	void GraphicsContext::ClearRenderTargetView(BackBuffer* backBuffer, const math::Color&  color)
	{
		mCommandList->ClearRenderTargetView(backBuffer->backBufferDescriptorHandle.cpuDescriptorHandle, color, 0u, nullptr);
	}
}

