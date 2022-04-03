#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"
#include "GFXUtils.hpp"

namespace helios::gfx
{
	class StructuredBuffer
	{
	public:
		template <typename T>
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor, std::span<const T> data, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE, std::wstring_view bufferName = "Structured Buffer")
		{
			auto gpuBuffer = gfx::utils::CreateGPUBuffer<T>(device, commandList, data, resourceFlag);
			m_DestinationResource = gpuBuffer.first;
			m_IntermediateResource = gpuBuffer.second;

			m_DestinationResource->SetName(bufferName.data());
			m_IntermediateResource->SetName(bufferName.data());

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
			{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer
				{
					.FirstElement = 0u,
					.NumElements = static_cast<UINT>(data.size()),
					.StructureByteStride = sizeof(T)
				}
			};

			device->CreateShaderResourceView(m_DestinationResource.Get(), &srvDesc, srvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);

			m_SRVIndexInDescriptorHeap = srvDescriptor.GetCurrentDescriptorIndex();

			srvDescriptor.OffsetCurrentHandle();
		}

		ID3D12Resource* GetResource() const;

		uint32_t GetSRVIndex() const;

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_DestinationResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_IntermediateResource;

		uint32_t m_SRVIndexInDescriptorHeap{};
	};
}


