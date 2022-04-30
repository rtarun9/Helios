#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	struct NonHDRTextureData
	{
		std::wstring_view texturePath{};
		uint32_t mipLevels{};
		bool isSRGB{};
		void* data{};
	};

	struct HDRTextureData
	{
		std::wstring_view texturePath{};
		uint32_t mipLevels{};
		DXGI_FORMAT format{};
		float* data{};
	};

	struct UAVTextureData
	{
		uint32_t width{};
		uint32_t height{};
		uint32_t depthOrArraySize{};
		uint32_t mipLevels{};
		DXGI_FORMAT format{};
	};

	enum class TextureTypes : uint8_t
	{
		NonHDRTextureData,
		HDRTextureData,
		UAVTextureData
	};

	// The texture class is common to several type of textures : Non HDR, HDR, and TextureUAV (i.e textures with no initial data, which are later filled using shader's by using the texture as a 
	// UAV. Example use cases : cube map from equirect texture, irradiance texture etc.
	// Uses stbi_image to load texture from file (for NonHDR and HDR textures).
	class Texture
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, Descriptor& srvUavDescriptor, std::variant<NonHDRTextureData, HDRTextureData, UAVTextureData> textureData, std::wstring_view textureName);
		void CreateUAV(ID3D12Device* device, Descriptor& descriptor, uint32_t level);

		ID3D12Resource* const GetTextureResource() const { return m_Texture.Get(); }

		uint32_t GetTextureIndex() const { return m_TextureIndexInDescriptorHeap; };
		uint32_t GetUAVIndex(uint32_t level = 0u) const { return m_UAVIndicesInDescriptorHeap.at(level); }

		uint32_t GetMipLevels() const { return m_MipLevels; }

	private:
		int m_Width{};
		int m_Height{};
		uint32_t m_MipLevels{};

		std::variant<NonHDRTextureData, HDRTextureData, UAVTextureData> m_TextureData{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

		uint32_t m_TextureIndexInDescriptorHeap{};
		std::vector<uint32_t> m_UAVIndicesInDescriptorHeap{};
	};
}

