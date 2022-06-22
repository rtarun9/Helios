#pragma once

#include "Pch.hpp"

#include "Device.hpp"
#include "Descriptor.hpp"
#include "MemoryAllocator.hpp"

namespace helios::gfx
{
	// The engine heavily uses StructuredBuffer's instead of Vertex Buffer's.
	template <typename T>
	class StructuredBuffer
	{
	public:
		StructuredBuffer(Device* const device, std::span<const T> data, std::wstring_view bufferName)
		{
			ResourceCreationDesc resourceCreationDesc
			{
				.resourceDesc
				{
					.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
					.Alignment = 0u,
					.Width = data.size_bytes(),
					.Height = 1u,
					.DepthOrArraySize = 1u,
					.MipLevels = 0u,
					.Format = DXGI_FORMAT_UNKNOWN,
					.SampleDesc
					{
						.Count = 1u,
						.Quality = 0u
					},
					.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
					.Flags = D3D12_RESOURCE_FLAG_NONE
				},
				.size = data.size_bytes(),
				.isCPUVisible = false,
			};

			mAllocation = device->GetMemoryAllocator()->CreateResource(resourceCreationDesc, bufferName);

			std::unique_ptr<UploadAllocation> uploadAllocation = device->GetUploadContext()->Allocate(data.size_bytes());
			uploadAllocation->Update(data);
			uploadAllocation->CopyResource(mAllocation.get());
			device->GetUploadContext()->ProcessUploadAllocations(std::move(uploadAllocation));
			
			// Data transfer (to Upload allocation -> To GPU only buffer).
			
			SRVCreationDesc srvCreationDesc
			{
				.srvDesc
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
				}
			};

			mSRVIndexInDescriptorHeap = device->CreateSRV(srvCreationDesc, mAllocation->resource.Get());
		}

		uint32_t GetSRVIndex() const {return mSRVIndexInDescriptorHeap;};

	private:
		std::unique_ptr<Allocation> mAllocation{};
		uint32_t mSRVIndexInDescriptorHeap{};
	};
}


