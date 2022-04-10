#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	class TextureUAV
	{
	public:
		void Init(ID3D12Device* device, Descriptor& srvUAVDescriptor, uint32_t width, uint32_t height, uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view textureUAVName);

		ID3D12Resource* GetTextureResource() const;

		uint32_t GetTextureIndex() const;
		uint32_t GetUAVIndex() const;

	private:
		void* m_TextureData{};
		int m_Width{};
		int m_Height{};
		int m_ComponentCount{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

		uint32_t m_TextureIndexInDescriptorHeap{};
		uint32_t m_UAVIndexInDescriptorHeap{};
	};
}


