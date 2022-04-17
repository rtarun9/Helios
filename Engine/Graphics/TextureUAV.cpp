#include "Pch.hpp"

#include "TextureUAV.hpp"

namespace helios::gfx
{
	void TextureUAV::Init(ID3D12Device* device, Descriptor& srvUAVDescriptor, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view textureUAVName)
	{
		D3D12_RESOURCE_DESC resourceDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = width,
			.Height = height,
			.DepthOrArraySize = static_cast<UINT16>(depth),
			.MipLevels = static_cast<UINT16>(mipLevels),
			.Format = format,
			.SampleDesc
			{
				.Count = 1u,
				.Quality = 0u
			},
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		};

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_Texture)));

		m_Texture->SetName(textureUAVName.data());

		// Create SRV for the texture 
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		switch (depth)
		{
		case 1:
		{
			srvDesc =
			{
				.Format = format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D
				{
					.MostDetailedMip = 0,
					.MipLevels = 1u,
				}
			};
		
			break;
		}
		
		case 6:
		{
			srvDesc =
			{
				.Format = format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.TextureCube
				{
					.MostDetailedMip = 0,
					.MipLevels = 1u
				}	
			};
			break;
		}
		}
		device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvUAVDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
		m_TextureIndexInDescriptorHeap = srvUAVDescriptor.GetCurrentDescriptorIndex();
		srvUAVDescriptor.OffsetCurrentHandle();

		// Create UAV for texture.
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

		switch (depth)
		{
		case 1:
		{
			uavDesc =
			{
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D
				{
					.MipSlice = 0u
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
		
		device->CreateUnorderedAccessView(m_Texture.Get(), nullptr, &uavDesc, srvUAVDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
		m_UAVIndexInDescriptorHeap = srvUAVDescriptor.GetCurrentDescriptorIndex();

		m_UAVIndices.push_back(m_UAVIndexInDescriptorHeap);

		srvUAVDescriptor.OffsetCurrentHandle();
	}

	void TextureUAV::CreateUAV(ID3D12Device* device, Descriptor& descriptor, uint32_t level)
	{
		auto resourceDesc = m_Texture->GetDesc();

		auto handle = descriptor.GetCurrentDescriptorHandle();

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
		m_UAVIndices.push_back(descriptor.GetDescriptorIndex(handle));

		descriptor.OffsetCurrentHandle();
	}

	ID3D12Resource* TextureUAV::GetTextureResource() const
	{
		return m_Texture.Get();
	}

	uint32_t TextureUAV::GetTextureIndex() const
	{
		return m_TextureIndexInDescriptorHeap;
	}

	uint32_t TextureUAV::GetUAVIndex(uint32_t level) const
	{
		if (level == 0u)
		{
			return m_UAVIndexInDescriptorHeap;
		}

		else
		{
			return m_UAVIndices.at(level);
		}
	}
}
