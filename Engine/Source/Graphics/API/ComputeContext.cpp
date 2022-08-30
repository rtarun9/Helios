
#include "ComputeContext.hpp"

#include "Device.hpp"

namespace helios::gfx
{
	ComputeContext::ComputeContext(Device* device, const gfx::PipelineState* pipelineState) : mDevice(*device)
	{
		mCommandList = device->GetComputeCommandQueue()->GetCommandList(pipelineState);

		// As all compute context's require to set the descriptor heap before hand, the user has option to set them manually (for explicitness) or just let the constructor take care of this.
		SetDescriptorHeaps(mDevice.GetSrvCbvUavDescriptor());

		mCommandList->SetComputeRootSignature(PipelineState::rootSignature.Get());
	}

	void ComputeContext::Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) const
	{
		mCommandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);
	}

	void ComputeContext::SetDescriptorHeaps(const Descriptor* const descriptor) const
	{
		std::array<ID3D12DescriptorHeap*, 1>  descriptorHeaps
		{
			descriptor->GetDescriptorHeap(),
		};

		mCommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
	}

	void ComputeContext::SetComputeRootSignature(PipelineState* pipelineState) const
	{
		mCommandList->SetComputeRootSignature(pipelineState->rootSignature.Get());
	}

	void ComputeContext::Set32BitComputeConstants(const void* renderResources) const
	{
		mCommandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, renderResources, 0u);
	}

	void ComputeContext::SetComputePipelineState(PipelineState* pipelineState) const
	{
		mCommandList->SetComputeRootSignature(pipelineState->rootSignature.Get());
		mCommandList->SetPipelineState(pipelineState->pipelineStateObject.Get());
	}

	void ComputeContext::AddResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState)
	{
		mResourceBarriers.push_back({ CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState) });
	}

	void ComputeContext::ExecuteResourceBarriers()
	{
		mCommandList->ResourceBarrier(static_cast<UINT>(mResourceBarriers.size()), mResourceBarriers.data());
		mResourceBarriers.clear();
	}
}