#include "Pch.hpp"

#include "ComputeContext.hpp"

#include "Device.hpp"

namespace helios::gfx
{
	ComputeContext::ComputeContext(Device& device) : mDevice(device)
	{
		mCommandList = device.GetGraphicsCommandQueue()->GetCommandList();

		// As all compute context's require to set the descriptor heap before hand, the user has option to set them manually (for explicitness) or just let the constructor take care of this.
		SetDescriptorHeaps(mDevice.GetSrvCbvUavDescriptor());
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
}