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

	std::unique_ptr<Allocation> MemoryAllocator::CreateResource(const ResourceCreationDesc& resourceCreationDesc, std::wstring_view resourceName)
	{
		Allocation allocation{};

		D3D12_RESOURCE_STATES resourceState{D3D12_RESOURCE_STATE_COPY_DEST};
		D3D12_HEAP_TYPE heapType{D3D12_HEAP_TYPE_DEFAULT};

		if (resourceCreationDesc.isCPUVisible)
		{
			resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			heapType = D3D12_HEAP_TYPE_UPLOAD;

		}

		D3D12MA::ALLOCATION_DESC allocationDesc
		{
			.HeapType = heapType
		};

		ThrowIfFailed(mAllocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState, nullptr, &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));
		
		if (resourceCreationDesc.isCPUVisible)
		{
			ThrowIfFailed(allocation.resource->Map(0u, nullptr, &allocation.mappedPointer.value()));
		}

		return std::move(std::make_unique<Allocation>(allocation));
	}
}