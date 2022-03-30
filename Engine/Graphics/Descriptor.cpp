#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	void Descriptor::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount, std::wstring_view descriptorName)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc
		{
			.Type = descriptorHeapType,
			.NumDescriptors = descriptorCount,
			.Flags = heapFlags,
			.NodeMask = 0u
		};

		ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
		m_DescriptorHeap->SetName(descriptorName.data());

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

		m_CurrentDescriptorHandle.cpuDescriptorHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		if (heapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			m_CurrentDescriptorHandle.gpuDescriptorHandle = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		}
	}

	ID3D12DescriptorHeap* Descriptor::GetDescriptorHeap()
	{
		return m_DescriptorHeap.Get();
	}

	uint32_t Descriptor::GetDescriptorSize()
	{
		return m_DescriptorSize;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Descriptor::GetCPUDescriptorHandleForStart()
	{
		return m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Descriptor::GetGPUDescriptorHandleForStart()
	{
		return m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Descriptor::GetCurrentCPUDescriptorHandle()
	{
		return 	m_CurrentDescriptorHandle.cpuDescriptorHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Descriptor::GetCurrentGPUDescriptorHandle()
	{
		return 	m_CurrentDescriptorHandle.gpuDescriptorHandle;
	}

	void Descriptor::Offset(D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset)
	{
		handle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::Offset(D3D12_GPU_DESCRIPTOR_HANDLE& handle, uint32_t offset)
	{
		handle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::Offset(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle, uint32_t offset)
	{
		cpuHandle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
		gpuHandle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::OffsetCurrentCPUDescriptor(uint32_t offset)
	{
		m_CurrentDescriptorHandle.cpuDescriptorHandle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::OffsetCurrentGPUDescriptor(uint32_t offset)
	{
		m_CurrentDescriptorHandle.gpuDescriptorHandle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::OffsetCurrentDescriptorHandles(uint32_t offset)
	{
		OffsetCurrentCPUDescriptor(offset);
		OffsetCurrentGPUDescriptor(offset);
	}
}
