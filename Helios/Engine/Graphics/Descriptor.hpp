#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	// Note : This function is heavily WIP.
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

		// Used to offset the interally stored CPU / GPU descriptor handle.
		void OffsetCurrentCPUDescriptor(uint32_t offset = 1u);
		void OffsetCurrentGPUDescriptor(uint32_t offset = 1u);
		void OffsetCurrentDescriptorHandles(uint32_t offset = 1u);

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
		uint32_t m_DescriptorSize{};

		// Note : The purpose for this is so that I can pass the descriptor directly into textures and other classes that use either the CPU or GPU descriptor handle.
		// This is only used for initialization of gfx object's as of now.
		D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentCPUDescriptorHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_CurrentGPUDescriptorHandle{};
	};
}
