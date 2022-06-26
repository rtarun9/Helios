#include "Pch.hpp"

#include "MemoryAllocator.hpp"

namespace helios::gfx
{
	MemoryAllocator::MemoryAllocator(ID3D12Device* device, IDXGIAdapter* adapter)
	{
		// Create D3D12MA adapter.
		D3D12MA::ALLOCATOR_DESC allocatorDesc
		{
			.pDevice = device,
			.pAdapter = adapter
		};

		ThrowIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &mAllocator));
	}

	std::unique_ptr<Allocation> MemoryAllocator::CreateResourceAllocation(const BufferCreationDesc& bufferCreationDesc, const ResourceCreationDesc& resourceCreationDesc)
	{
		Allocation allocation{};

		D3D12_RESOURCE_STATES resourceState{D3D12_RESOURCE_STATE_COMMON};
		D3D12_HEAP_TYPE heapType{D3D12_HEAP_TYPE_DEFAULT};
		bool isCpuVisible{};

		switch (bufferCreationDesc.usage)
		{
			case BufferUsage::UploadBuffer:
			case BufferUsage::ConstantBuffer:
			{
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
				heapType = D3D12_HEAP_TYPE_UPLOAD;
				isCpuVisible = true;
			}break;

			case BufferUsage::IndexBuffer:
			case BufferUsage::StructuredBuffer:
			{
				resourceState = D3D12_RESOURCE_STATE_COMMON;
				heapType = D3D12_HEAP_TYPE_DEFAULT;
				isCpuVisible = false;
			}break;
		};

		D3D12MA::ALLOCATION_DESC allocationDesc
		{
			.HeapType = heapType
		};


		ThrowIfFailed(mAllocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState, nullptr, &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));
		
		if (isCpuVisible)
		{
			ThrowIfFailed(allocation.resource->Map(0u, nullptr, &allocation.mappedPointer));
		}

		allocation.resource->SetName(bufferCreationDesc.name.c_str());

		return std::move(std::make_unique<Allocation>(allocation));
	}
}