#pragma once

#include "Pch.hpp"

#include "IndexBuffer.hpp"
#include "Device.hpp"

namespace helios::gfx
{
	void IndexBuffer::Init(Device* const device, MemoryAllocator* const memoryAllocator, std::span<const uint32_t> data, std::wstring_view indexBufferName)
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
				.Format = DXGI_FORMAT_R32_UINT,
				.SampleDesc
				{
					.Count = 1u,
					.Quality = 0u
				},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_NONE
			}
		};

		mAllocation = memoryAllocator->CreateResource(resourceCreationDesc, indexBufferName);

		mBufferView =
		{
			.BufferLocation = mAllocation->resource->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(data.size_bytes()),
			.Format = DXGI_FORMAT_R32_UINT
		};

		device->getupload
	}
}