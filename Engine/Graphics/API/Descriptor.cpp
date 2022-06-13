#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	Descriptor::Descriptor(ID3D12Device* const device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount, std::wstring_view descriptorName)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc
		{
			.Type = descriptorHeapType,
			.NumDescriptors = descriptorCount,
			.Flags = heapFlags,
			.NodeMask = 0u
		};

		ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));
		mDescriptorHeap->SetName(descriptorName.data());

		mDescriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

		mDescriptorHandleFromStart.cpuDescriptorHandle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		if (heapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			mDescriptorHandleFromStart.gpuDescriptorHandle = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		}

		mDescriptorHandleFromStart.descriptorSize = mDescriptorSize;

		mCurrentDescriptorHandle = mDescriptorHandleFromStart;
	}

	DescriptorHandle Descriptor::GetDescriptorHandleFromIndex(uint32_t index) const
	{
		DescriptorHandle handle = GetDescriptorHandleForStart();
		Offset(handle, index);

		return handle;
	}

	uint32_t Descriptor::GetDescriptorIndex(const DescriptorHandle& descriptorHandle) const
	{
		return static_cast<uint32_t>((descriptorHandle.gpuDescriptorHandle.ptr - mDescriptorHandleFromStart.gpuDescriptorHandle.ptr) / mDescriptorSize);
	}

	uint32_t Descriptor::GetCurrentDescriptorIndex() const
	{
		return GetDescriptorIndex(mCurrentDescriptorHandle);
	}

	void Descriptor::Offset(D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset) const
	{
		handle.ptr += mDescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::Offset(D3D12_GPU_DESCRIPTOR_HANDLE& handle, uint32_t offset) const
	{
		handle.ptr += mDescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::Offset(DescriptorHandle& descriptorHandle, uint32_t offset) const
	{
		descriptorHandle.cpuDescriptorHandle.ptr += mDescriptorSize * static_cast<unsigned long long>(offset);
		descriptorHandle.gpuDescriptorHandle.ptr += mDescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::OffsetCurrentHandle(uint32_t offset)
	{
		Offset(mCurrentDescriptorHandle, offset);
	}
}
