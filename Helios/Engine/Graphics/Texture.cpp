#include "Pch.hpp"

#include "Texture.hpp"
#include "GFXUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace helios::gfx
{
	void Texture::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE srvCPUDescriptor, std::wstring_view texturePath)
	{
		m_TextureData = stbi_loadf(WstringToString(texturePath).c_str(), &m_Width, &m_Height, &m_ComponentCount, 4);

		D3D12_RESOURCE_DESC textureDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0u,
			.Width = static_cast<UINT>(m_Width),
			.Height = static_cast<UINT>(m_Height),
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		// Create intermediate resoruce to place texture in GPU accesible memory.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_Texture)));

		// Create GPU buffer for texture

		const uint64_t uploadBufferSize = GetRequiredIntermediateSize(m_Texture.Get(), 0u, 1u);

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_TextureUploadHeap)));

		// Copy data from intermediate buffer to the upload heap.
		D3D12_SUBRESOURCE_DATA textureSubresourceData
		{
			.pData = m_TextureData,
			.RowPitch = static_cast<__int64>(m_Width * m_ComponentCount),
			.SlicePitch = static_cast<__int64>(m_Height * m_Width* m_ComponentCount)
		};

		UpdateSubresources(commandList, m_Texture.Get(), m_TextureUploadHeap.Get(), 0u, 0u, 1u, &textureSubresourceData);

		// Transition resource from copy dest to Pixel SRV.
		gfx::utils::TransitionResource(commandList, m_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Create SRV for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = textureDesc.Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MipLevels = 1
			}
		};

		device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvCPUDescriptor);
	}

	ID3D12Resource* Texture::GetTextureResource()
	{
		return m_Texture.Get();
	}
}