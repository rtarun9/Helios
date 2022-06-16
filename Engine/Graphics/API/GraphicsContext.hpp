#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	class Device;

	// Wrapper class for CommandList, which provides a set of easy and simple functions to record commands for execution by GPU.
	// The command queue will contain a queue of command list, which can be passed into the GraphicsContext's constructor to create a GraphicsContext class.
	// note (rtarun9) : This design is subject to change.
	class GraphicsContext
	{
	public:
		GraphicsContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		ID3D12GraphicsCommandList* const GetCommandList() { return mCommandList.Get(); }

		// Core functionalities.
		void AddResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState);
		
		void ClearRenderTargetView(BackBuffer* backBuffer, const DirectX::SimpleMath::Color& color);

	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	};
}


