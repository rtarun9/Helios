#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{

	class Texture
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor, std::wstring_view path, std::wstring_view textureName, bool isSRGB = true);

		ID3D12Resource* GetTextureResource() const;

		uint32_t GetTextureIndex() const;

	private:
		void* m_TextureData{};
		int m_Width{};
		int m_Height{};
		int m_ComponentCount{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

		uint32_t m_TextureIndexInDescriptorHeap{};
	};
}

