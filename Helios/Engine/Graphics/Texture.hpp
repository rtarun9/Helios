#pragma once

#include "Pch.hpp"

namespace helios::gfx
{

	class Texture
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE srvCPUDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE srvGPUDescriptor, std::wstring_view path, bool isSRGB = true);

		ID3D12Resource* GetTextureResource();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle();

	private:
		void* m_TextureData{};
		int m_Width{};
		int m_Height{};
		int m_ComponentCount{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUDescriptorHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptorHandle{};

		// This will be used to be able to view into the descriptor and get its CPU / GPU descriptor handle.
		uint32_t m_DescriptorOffset{};
	};
}

