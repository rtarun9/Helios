#pragma once

#include "PipelineState.hpp"
#include "Resources.hpp"
#include "Descriptor.hpp"


namespace helios::gfx
{
	// Base class for Context (i.e wrapper for command list). Provides batching of resource barriers for optimal performance.
	// It uses a GraphicsCOmmandList and is can execute commands of any type (copy, compute, graphics, etc).

	// Source : https://docs.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles.
	class Context
	{
	public:
		ID3D12GraphicsCommandList1* const GetCommandList() const { return mCommandList.Get(); }

		// Core functionalities to be inherited by Graphics/Compute context.

		// Resource related functions : 
		void AddResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState);
		void AddResourceBarrier(std::span<const RenderTarget*> renderTargets, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState);
	
		// Execute all resource barriers.
		void ExecuteResourceBarriers();
	
	protected:
		Context() = default;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> mCommandList{};

		// Resource barriers are quite heavy, and executing them in a single call (batched) is very efficient.
		// The resource barriers are executed when the ExecuteResourceBarriers() call is invoked, which must happen before command list is sent over to the
		// device for execution, or be batched as much as possible.

		std::vector<CD3DX12_RESOURCE_BARRIER> mResourceBarriers{};
	};
}