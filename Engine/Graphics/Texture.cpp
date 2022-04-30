#include "Pch.hpp"

#include "Texture.hpp"
#include "GFXUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace helios::gfx
{
	void Texture::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, Descriptor& srvUavDescriptor, std::variant<NonHDRTextureData, HDRTextureData, UAVTextureData> textureData, std::wstring_view textureName)
	{
		switch (textureData.index())
		{
			case EnumClassValue(TextureTypes::NonHDRTextureData) :
			{
				NonHDRTextureData data = std::get<NonHDRTextureData>(textureData);
				
				stbi_set_flip_vertically_on_load(true);

				int componentCount{};
				data.data = stbi_load(WstringToString(data.texturePath).c_str(), &m_Width, &m_Height, &componentCount, 4u);
				componentCount = 4;

				DXGI_FORMAT textureFormat = data.isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

				D3D12_RESOURCE_DESC textureDesc
				{
					.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					.Alignment = 0u,
					.Width = static_cast<UINT>(m_Width),
					.Height = static_cast<UINT>(m_Height),
					.DepthOrArraySize = 1,
					.MipLevels = static_cast<UINT16>(data.mipLevels),
					.Format = textureFormat,
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
					.pData = data.data,
					.RowPitch = static_cast<__int64>(m_Width * componentCount),
					.SlicePitch = static_cast<__int64>(m_Height * m_Width * componentCount)
				};

				UpdateSubresources(commandList, m_Texture.Get(), m_TextureUploadHeap.Get(), 0u, 0u, 1u, &textureSubresourceData);

				m_Texture->SetName(textureName.data());

				std::wstring intermediateBufferName{textureName.data() + std::wstring(L" Intermediate Buffer")};
				m_TextureUploadHeap->SetName(intermediateBufferName.c_str());

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
						.MipLevels = data.mipLevels
					}
				};

				device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvUavDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);

				m_TextureIndexInDescriptorHeap = srvUavDescriptor.GetCurrentDescriptorIndex();

				srvUavDescriptor.OffsetCurrentHandle();

				m_UAVIndicesInDescriptorHeap.reserve(0u);

				m_TextureData = data;
			}
			break;

			case EnumClassValue(TextureTypes::HDRTextureData):
			{
				HDRTextureData data = std::get<HDRTextureData>(textureData);
				stbi_set_flip_vertically_on_load(true);

				int componentCount{};
				data.data = stbi_loadf(WstringToString(data.texturePath).c_str(), &m_Width, &m_Height, &componentCount, 4u);
				componentCount = 4;

				D3D12_RESOURCE_DESC textureDesc
				{
					.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					.Alignment = 0u,
					.Width = static_cast<UINT>(m_Width),
					.Height = static_cast<UINT>(m_Height),
					.DepthOrArraySize = 1,
					.MipLevels = static_cast<UINT16>(data.mipLevels),
					.Format = data.format,
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
					.pData = data.data,
					.RowPitch = static_cast<__int64>(m_Width * componentCount * 4),
					.SlicePitch = static_cast<__int64>(m_Height * m_Width * componentCount * 4)
				};

				UpdateSubresources(commandList, m_Texture.Get(), m_TextureUploadHeap.Get(), 0u, 0u, 1u, &textureSubresourceData);

				m_Texture->SetName(textureName.data());

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
						.MipLevels = data.mipLevels
					}
				};

				device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvUavDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);

				m_TextureIndexInDescriptorHeap = srvUavDescriptor.GetCurrentDescriptorIndex();
				m_UAVIndicesInDescriptorHeap.reserve(data.mipLevels);
				srvUavDescriptor.OffsetCurrentHandle();

				m_TextureData = data;
			}
			break;

			case EnumClassValue(TextureTypes::UAVTextureData):
			{
				UAVTextureData data = std::get<UAVTextureData>(textureData);

				D3D12_RESOURCE_DESC resourceDesc
				{
					.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					.Width = data.width,
					.Height = data.height,
					.DepthOrArraySize = static_cast<UINT16>(data.depthOrArraySize),
					.MipLevels = static_cast<UINT16>(data.mipLevels),
					.Format = data.format,
					.SampleDesc
					{
						.Count = 1u,
						.Quality = 0u
					},
					.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
				};

				CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
				ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_Texture)));

				m_Texture->SetName(textureName.data());

				// Create SRV for the texture 
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
				switch (data.depthOrArraySize)
				{
				case 1:
				{
					srvDesc =
					{
						.Format = data.format,
						.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
						.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
						.Texture2D
						{
							.MostDetailedMip = 0,
							.MipLevels = data.mipLevels,
						}
					};

					break;
				}

				case 6:
				{
					srvDesc =
					{
						.Format = data.format,
						.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
						.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
						.TextureCube
						{
							.MostDetailedMip = 0u,
							.MipLevels = data.mipLevels 
						}
					};
					break;
				}
				}

				device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvUavDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
				m_TextureIndexInDescriptorHeap = srvUavDescriptor.GetCurrentDescriptorIndex();
				srvUavDescriptor.OffsetCurrentHandle();

				m_UAVIndicesInDescriptorHeap.reserve(data.mipLevels);

				// Create UAV for texture.
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

				switch (data.depthOrArraySize)
				{
				case 1:
				{
					uavDesc =
					{
						.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
						.Texture2D
						{
							.MipSlice = 0u,
						}
					};
					break;
				}

				case 6:
				{
					uavDesc =
					{
						.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
						.Texture2DArray
						{
							.MipSlice = 0u,
							.FirstArraySlice = 0u,
							.ArraySize = 6u
						}
					};
					break;
				}
				}

				device->CreateUnorderedAccessView(m_Texture.Get(), nullptr, &uavDesc, srvUavDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
				m_UAVIndicesInDescriptorHeap.push_back(srvUavDescriptor.GetCurrentDescriptorIndex());

				srvUavDescriptor.OffsetCurrentHandle();

				m_MipLevels = data.mipLevels;
				m_TextureData = data;
			}
			break;
		}
	}

	void Texture::CreateUAV(ID3D12Device* const device, Descriptor& descriptor, uint32_t level)
	{
		if (level == 0)
		{
			return;
		}

		D3D12_RESOURCE_DESC resourceDesc = m_Texture->GetDesc();

		gfx::DescriptorHandle handle = descriptor.GetCurrentDescriptorHandle();

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

		switch (resourceDesc.DepthOrArraySize)
		{
		case 1:
		{
			uavDesc =
			{
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D
				{
					.MipSlice = level
				}
			};
			break;
		}

		case 6:
		{
			uavDesc =
			{
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
				.Texture2DArray
				{
					.MipSlice = level,
					.FirstArraySlice = 0u,
					.ArraySize = 6u
				}
			};
			break;
		}
		}

		device->CreateUnorderedAccessView(m_Texture.Get(), nullptr, &uavDesc, handle.cpuDescriptorHandle);
		m_UAVIndicesInDescriptorHeap.push_back(descriptor.GetDescriptorIndex(handle));

		descriptor.OffsetCurrentHandle();
	}
}
