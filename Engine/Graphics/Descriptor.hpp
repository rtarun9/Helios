#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

		uint32_t descriptorSize{};

		void Offset()
		{
			cpuDescriptorHandle.ptr += descriptorSize;
			gpuDescriptorHandle.ptr += descriptorSize;
		}
	};

	class Descriptor
	{
	public:
		void Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount, std::wstring_view descriptorName);
		
		ID3D12DescriptorHeap* GetDescriptorHeap() const;
		uint32_t GetDescriptorSize() const;

		DescriptorHandle GetDescriptorHandleForStart() const;
		DescriptorHandle GetCurrentDescriptorHandle() const;
		DescriptorHandle GetDescriptorHandleFromIndex(uint32_t index) const;

		// Returns a index that can be used to directly index into a descriptor heap.
		uint32_t GetDescriptorIndex(const DescriptorHandle& descriptorHandle) const;
		uint32_t GetCurrentDescriptorIndex() const;
	
		// Used to offset a X_Handle passed into function.
		void Offset(D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset = 1u) const;
		void Offset(D3D12_GPU_DESCRIPTOR_HANDLE& handle, uint32_t offset = 1u) const;
		void Offset(DescriptorHandle& handle, uint32_t offset = 1u) const;

		void OffsetCurrentHandle(uint32_t offset = 1u);

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
		uint32_t m_DescriptorSize{};

		DescriptorHandle m_DescriptorHandleFromStart{};
		DescriptorHandle m_CurrentDescriptorHandle{};
	};
}
