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

		m_DescriptorHandleFromStart.cpuDescriptorHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		if (heapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			m_DescriptorHandleFromStart.gpuDescriptorHandle = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		}

		m_DescriptorHandleFromStart.descriptorSize = m_DescriptorSize;

		m_CurrentDescriptorHandle = m_DescriptorHandleFromStart;
	}

	ID3D12DescriptorHeap* Descriptor::GetDescriptorHeap() const
	{
		return m_DescriptorHeap.Get();
	}

	uint32_t Descriptor::GetDescriptorSize() const
	{
		return m_DescriptorSize;
	}

	DescriptorHandle Descriptor::GetDescriptorHandleForStart() const
	{
		return m_DescriptorHandleFromStart;
	}

	DescriptorHandle Descriptor::GetCurrentDescriptorHandle() const
	{
		return m_CurrentDescriptorHandle;
	}

	DescriptorHandle Descriptor::GetDescriptorHandleFromIndex(uint32_t index) const
	{
		DescriptorHandle handle= GetDescriptorHandleForStart();
		Offset(handle, index);

		return handle;
	}

	uint32_t Descriptor::GetDescriptorIndex(const DescriptorHandle& descriptorHandle) const
	{
		return static_cast<uint32_t>((descriptorHandle.gpuDescriptorHandle.ptr - m_DescriptorHandleFromStart.gpuDescriptorHandle.ptr) / m_DescriptorSize);
	}

	uint32_t Descriptor::GetCurrentDescriptorIndex() const
	{
		return GetDescriptorIndex(m_CurrentDescriptorHandle);
	}

	void Descriptor::Offset(D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset) const
	{
		handle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::Offset(D3D12_GPU_DESCRIPTOR_HANDLE& handle, uint32_t offset) const
	{
		handle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::Offset(DescriptorHandle& descriptorHandle, uint32_t offset) const
	{
		descriptorHandle.cpuDescriptorHandle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
		descriptorHandle.gpuDescriptorHandle.ptr += m_DescriptorSize * static_cast<unsigned long long>(offset);
	}

	void Descriptor::OffsetCurrentHandle(uint32_t offset)
	{
		Offset(m_CurrentDescriptorHandle, offset);
	}
}
