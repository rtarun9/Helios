#include "Context.hpp"

namespace helios::gfx
{
	void Context::AddResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState)
	{
		mResourceBarriers.push_back({ CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState) });
	}

	void Context::AddResourceBarrier(std::span<const RenderTarget*> renderTargets, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState)
	{
		for (const auto& rt : renderTargets)
		{
			mResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(rt->renderTexture->allocation->resource.Get(), previousState, newState));
		}
	}

	void Context::ExecuteResourceBarriers()
	{
		mCommandList->ResourceBarrier(static_cast<UINT>(mResourceBarriers.size()), mResourceBarriers.data());
		mResourceBarriers.clear();
	}
}