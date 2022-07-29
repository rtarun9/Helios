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

	// Descriptor abstraction that has a 'current descriptor' that comes in handy while initializing resource (Texture's, buffer's etc).
	// Has methods to return index of current descriptor : most resource abstarctions (texture's, buffer's) etc store this index and use for bindless rendering.
	class Descriptor
	{
	public:
		Descriptor(ID3D12Device* const device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount, std::wstring_view descriptorName);

		ID3D12DescriptorHeap* const GetDescriptorHeap() const { return mDescriptorHeap.Get(); }
		uint32_t GetDescriptorSize() const { return mDescriptorSize; };

		DescriptorHandle GetDescriptorHandleFromStart() const { return mDescriptorHandleFromStart; };
		DescriptorHandle GetCurrentDescriptorHandle() const { return mCurrentDescriptorHandle; };

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
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap{};
		uint32_t mDescriptorSize{};

		DescriptorHandle mDescriptorHandleFromStart{};
		DescriptorHandle mCurrentDescriptorHandle{};
	};
}
