#include "Pch.hpp"

#include "Include/Graphics/Descriptor.hpp"

namespace helios::gfx
{
	void Descriptor::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc
		{
			.Type = descriptorHeapType,
			.NumDescriptors = descriptorCount,
			.Flags = heapFlags,
			.NodeMask = 0u
		};

		ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);
	}

	ID3D12DescriptorHeap* Descriptor::GetDescriptorHeap()
	{
		return m_DescriptorHeap.Get();
	}

	uint32_t Descriptor::GetDescriptorSize()
	{
		return m_DescriptorSize;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Descriptor::GetCPUDescriptorHandle()
	{
		return m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Descriptor::GetGPUDescriptorHandle()
	{
		return m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}
}
