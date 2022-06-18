#pragma once

#include "Pch.hpp"

#include "PipelineState.hpp"
#include "Descriptor.hpp"
#include "UploadContext.hpp"

namespace helios::gfx::utils
{
	// Helper function to create a Placed GPU Buffer.
	template <typename T>
	inline ID3D12Resource* CreateBuffer(ID3D12Device* const device, UploadContext* const uploadContext, std::optional<std::span<const T>> data, ID3D12Heap* heap, uint32_t heapOffset, bool isCPUVisible = false, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
	{
		D3D12_RESOURCE_DESC bufferResouceDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0u,
			.Width = data.value().size_bytes(),
			.DepthOrArraySize = 1u,
			.MipLevels = 1u,
			.Format = DXGI_FORMAT_UNKNOWN,
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = isCPUVisible ? D3D12_RESOURCE_FLAG_NONE : flags
		};

		ID3D12Resource* resource{ nullptr };
		D3D12_RESOURCE_STATES resourceState{ isCPUVisible ? D3D12_RESOURCE_STATE_GENERIC_READ : state };

		if (heap)
		{
			ThrowIfFailed(device->CreatePlacedResource(heap, heapOffset, &bufferResouceDesc, resourceState, nullptr, IID_PPV_ARGS(&heap)));
		}
		else
		{
			// If heap is nullptr, we create a commited resource, which will create a implicit heap and a resource within it.
			CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
			CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

			CD3DX12_HEAP_PROPERTIES heapProperties = isCPUVisible ? defaultHeapProperties :  uploadHeapProperties;
			ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferResouceDesc, resourceState, nullptr, IID_PPV_ARGS(&resource)));
		}

		if (data.has_value())
		{
			// Map memory of buffer to system memory so we can directly write to it.
			if (isCPUVisible)
			{
				D3D12_RANGE range{};
				void* cpuAddress{ nullptr };
				ThrowIfFailed(resource->Map(0u, &range, reinterpret_cast<void**>(&cpuAddress)));
				memcpy(cpuAddress, data.value().data(), data.value().size_bytes());
				resource->Unmap(0u, nullptr);
			}
			else
			{
				// Upload context required if we want resource to be in GPU only memory.
				memcpy(uploadContext->GetCPUAddress(), data.value().data(), data.value().size_bytes());
				uploadContext->GetCommandList()->CopyResource(resource, uploadContext->GetResource());
			}
		}
		
		return resource;
	}

	inline void ClearDepthBuffer(ID3D12GraphicsCommandList* const commandList, DescriptorHandle& dsvHandle, float depthClearValue = 1.0f)
	{
		commandList->ClearDepthStencilView(dsvHandle.cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, depthClearValue, 0u, 0u, nullptr);
	}

	inline void SetDescriptorHeaps(ID3D12GraphicsCommandList* commandList, Descriptor& descriptors)
	{
		std::array<ID3D12DescriptorHeap*, 1>  descriptorHeap
		{
			descriptors.GetDescriptorHeap()
		};

		commandList->SetDescriptorHeaps(1u, descriptorHeap.data());
	}
}