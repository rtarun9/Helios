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
		
		ID3D12DescriptorHeap* GetDescriptorHeap();
		uint32_t GetDescriptorSize();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForStart();
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForStart();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentCPUDescriptorHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentGPUDescriptorHandle();

		// Used to offset a X_Handle passed into function.
		void Offset(D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset = 1u);
		void Offset(D3D12_GPU_DESCRIPTOR_HANDLE& handle, uint32_t offset = 1u);
		void Offset(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle, uint32_t offset = 1u);

		void OffsetCurrentHandle();

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
		uint32_t m_DescriptorSize{};

		DescriptorHandle m_DescriptorHandleFromStart{};
		DescriptorHandle m_CurrentDescriptorHandle{};
	};
}
