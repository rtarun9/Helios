#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	class Descriptor
	{
	public:
		void Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount);
		
		ID3D12DescriptorHeap* GetDescriptorHeap();
		uint32_t GetDescriptorSize();

		// As of now, both functions return XHandles for the start of the heap. For offset, use d3dx12.h's CD3DXHandle's offset function.
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle();

		void Offset(D3D12_CPU_DESCRIPTOR_HANDLE& handle);
		void Offset(D3D12_GPU_DESCRIPTOR_HANDLE& handle);
		void Offset(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle);

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
		uint32_t m_DescriptorSize{};
	};
}
