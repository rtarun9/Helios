#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	class RenderTarget;
	class Device;

	// Wrapper class for Graphics CommandList, which provides a set of easy and simple functions to record commands for execution by GPU.
	// The command queue will contain a queue of command list, which can be passed into the GraphicsContext's constructor to create a GraphicsContext object.
	// note (rtarun9) : This design is subject to change.
	class GraphicsContext
	{
	public:
		GraphicsContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		ID3D12GraphicsCommandList* const GetCommandList() { return mCommandList.Get(); }

		// Core functionalities.
		void AddResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState);
		
		void ClearRenderTargetView(BackBuffer* const backBuffer, const DirectX::SimpleMath::Color& color);

		void BindRenderTarget(RenderTarget* const renderTarget);

	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	};
}

