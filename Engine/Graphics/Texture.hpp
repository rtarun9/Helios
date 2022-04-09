#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{

	class Texture
	{
	public:
		// For textures of format R8G8B8A8_UNORM or its SRGB Equivalent.
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor, std::wstring_view texturePath, uint32_t mipLevels, bool isSRGB, std::wstring_view textureName);

		// For HDR textures.
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor, std::wstring_view texturePath, uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view textureName);

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

