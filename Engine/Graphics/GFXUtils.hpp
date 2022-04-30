#pragma once

#include "Pch.hpp"

#include "Material.hpp"
#include "Descriptor.hpp"

namespace helios::gfx::utils
{
	inline void TransitionResource(ID3D12GraphicsCommandList* const commandList, ID3D12Resource* const resource,
		D3D12_RESOURCE_STATES previouState, D3D12_RESOURCE_STATES newState)
	{
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, previouState, newState);
		commandList->ResourceBarrier(1u, &transitionBarrier);
	}

	inline void ClearRTV(ID3D12GraphicsCommandList* const commandList, DescriptorHandle& rtvHandle, std::span<const float> clearColor)
	{
		commandList->ClearRenderTargetView(rtvHandle.cpuDescriptorHandle, clearColor.data(), 0u, nullptr);
	}

	inline void ClearDepthBuffer(ID3D12GraphicsCommandList* const commandList, DescriptorHandle& dsvHandle, float depthClearValue = 1.0f)
	{
		commandList->ClearDepthStencilView(dsvHandle.cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, depthClearValue, 0u, 0u, nullptr);
	}

	// Create a GPU buffer that returns two buffers : The final Destination buffer and an intermediate buffer (that is used to transfer data from the CPU to the GPU).
	// The intermediate buffer needs to be in scope until the command's in the list have finished execution, hence why is it being returned along with destination buffer.
	template <typename T>
	inline std::pair<ID3D12Resource*, ID3D12Resource*> CreateGPUBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::span<const T> bufferData, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE)
	{
		if (bufferData.empty())
		{
			throw std::exception("Cannot create GPU Buffer for data of 0 bytes.");
		}

		ID3D12Resource* destinationResource{ nullptr };
		ID3D12Resource* intermediateResource{ nullptr };

		size_t bufferSize = bufferData.size_bytes();

		// Commited resource that acts as the destination resource.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperites(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resourceFlags);
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperites, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&destinationResource)));

		// Intermediate upload heap that is needed to transfer CPU buffer data into the GPU memory.
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC intermediateResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &intermediateResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource)));

		// Logic to transfer data from CPU to GPU.

		// TODo : Possible UB, since bufferData is not in scope while the commadn list is being executed, need to check this UB soon.
		D3D12_SUBRESOURCE_DATA subresourceData
		{
			.pData = bufferData.data(),
			.RowPitch = static_cast<long long>(bufferSize),
			.SlicePitch = static_cast<long long>(bufferSize)
		};

		UpdateSubresources(commandList, destinationResource, intermediateResource, 0u, 0u, 1u, &subresourceData);

		return { destinationResource, intermediateResource};
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