#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	class TextureUAV
	{
	public:
		void Init(ID3D12Device* device, Descriptor& srvUAVDescriptor, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view textureUAVName);
		
		void CreateUAV(ID3D12Device* device, Descriptor& descriptor, uint32_t level);

		ID3D12Resource* GetTextureResource() const;

		uint32_t GetTextureIndex() const;
		uint32_t GetUAVIndex(uint32_t level = 0u) const;

		uint32_t GetMipLevels() const;

	private:
		int m_Width{};
		int m_Height{};
		int m_ComponentCount{};

		uint32_t m_MipLevels{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

		uint32_t m_TextureIndexInDescriptorHeap{};
		uint32_t m_UAVIndexInDescriptorHeap{};

		// Note : This is only for textures that use mip maps (mipLevels > 0u).
		std::vector<uint32_t> m_UAVIndices{};
	};
}


