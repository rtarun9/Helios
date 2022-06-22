#include "Pch.hpp"

#include "GraphicsContext.hpp"
#include "RenderTarget.hpp"

namespace helios::gfx
{
	GraphicsContext::GraphicsContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
	{
		mCommandList = commandList;
	}

	void GraphicsContext::AddResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState);
		mCommandList->ResourceBarrier(1u, &resourceBarrier);
	}

	void GraphicsContext::ClearRenderTargetView(BackBuffer* const backBuffer, const math::Color&  color)
	{
		mCommandList->ClearRenderTargetView(backBuffer->backBufferDescriptorHandle.cpuDescriptorHandle, color, 0u, nullptr);
	}

	void GraphicsContext::BindRenderTarget(RenderTarget* const renderTarget)
	{
		auto rtIndexBufferView = RenderTarget::sIndexBuffer->GetBufferView();

		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mCommandList->IASetIndexBuffer(&rtIndexBufferView);
	}
}

