#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"
#include "UploadContext.hpp"

namespace helios::gfx
{
	// The engine heavily uses StructuredBuffer's instead of Vertex Buffer's.
	template <typename T>
	class StructuredBuffer
	{
	public:
		StructuredBuffer(ID3D12Device* const device, ID3D12Resource** bufferResource, ID3D12Resource** intermediateBufferResource, Descriptor* const srvDescriptor, std::span<const T> data, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE)
		{
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

			device->CreateShaderResourceView(*bufferResource, &srvDesc, srvDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);

			mSRVIndexInDescriptorHeap = srvDescriptor->GetCurrentDescriptorIndex();

			srvDescriptor->OffsetCurrentHandle();
		}

		uint32_t GetSRVIndex() const {return mSRVIndexInDescriptorHeap;};

	private:
		uint32_t mSRVIndexInDescriptorHeap{};
	};
}


