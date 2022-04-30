#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"
#include "GFXUtils.hpp"

namespace helios::gfx
{
	// The engine heavily uses StructuredBuffer's instead of Vertex Buffer's.
	template <typename T>
	class StructuredBuffer
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, Descriptor& srvDescriptor, std::span<const T> data, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE, std::wstring_view bufferName = "Structured Buffer")
		{
			std::pair<ID3D12Resource*, ID3D12Resource*> gpuBuffer =  gfx::utils::CreateGPUBuffer<T>(device, commandList, data, resourceFlag);

			m_DestinationResource = gpuBuffer.first;
			m_IntermediateResource = gpuBuffer.second;

			m_DestinationResource->SetName(bufferName.data());
			
			std::wstring intermediateBufferName = bufferName.data() + std::wstring(L" Intermediate Buffer");
			m_IntermediateResource->SetName(intermediateBufferName.c_str());

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

		ID3D12Resource* const GetResource() const { return m_DestinationResource.Get(); }

		uint32_t GetSRVIndex() const {return m_SRVIndexInDescriptorHeap;};

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_DestinationResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_IntermediateResource;

		uint32_t m_SRVIndexInDescriptorHeap{};
	};
}


