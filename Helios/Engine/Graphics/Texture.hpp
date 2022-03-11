#pragma once

#include "Pch.hpp"

namespace helios::gfx
{

	class Texture
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE srvCPUDescriptor, std::wstring_view path);

		ID3D12Resource* GetTextureResource();

	private:
		void* m_TextureData{};
		int m_Width{};
		int m_Height{};
		int m_ComponentCount{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;
	};
}

