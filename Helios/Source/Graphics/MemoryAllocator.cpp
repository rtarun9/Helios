#include "Graphics/MemoryAllocator.hpp"

#include "Graphics/Resources.hpp"

namespace helios::gfx
{
    MemoryAllocator::MemoryAllocator(ID3D12Device* const device, IDXGIAdapter* const adapter)
    {
        // Create D3D12MA adapter.
        const D3D12MA::ALLOCATOR_DESC allocatorDesc = {
            .pDevice = device,
            .pAdapter = adapter,
        };

        throwIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &m_allocator));
    }

    std::unique_ptr<Allocation> MemoryAllocator::createBufferResourceAllocation(
        const BufferCreationDesc& bufferCreationDesc, const ResourceCreationDesc& resourceCreationDesc)
    {
        Allocation allocation{};

        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
        bool isCpuVisible = false;

        switch (bufferCreationDesc.usage)
        {
        case BufferUsage::UploadBuffer:
        case BufferUsage::ConstantBuffer: {
            resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            heapType = D3D12_HEAP_TYPE_UPLOAD;
            isCpuVisible = true;
        }
        break;

        case BufferUsage::IndexBuffer:
        case BufferUsage::StructuredBuffer: {
            resourceState = D3D12_RESOURCE_STATE_COMMON;
            heapType = D3D12_HEAP_TYPE_DEFAULT;
            isCpuVisible = false;
        }
        break;
        };

        const D3D12MA::ALLOCATION_DESC allocationDesc = {
            .HeapType = heapType,
        };

        throwIfFailed(m_allocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState,
                                                 nullptr, &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));

        if (isCpuVisible)
        {
            // Give the mapped pointer some value to hold.
            allocation.mappedPointer = nullptr;
            throwIfFailed(allocation.resource->Map(0u, nullptr, &allocation.mappedPointer.value()));
        }

        allocation.resource->SetName(bufferCreationDesc.name.c_str());
        allocation.allocation->SetResource(allocation.resource.Get());

        return std::move(std::make_unique<Allocation>(allocation));
    }
} // namespace helios::gfx