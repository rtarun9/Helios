#pragma once

#include "Resources.hpp"

namespace helios::gfx
{
    // Memory allocator handles allocation of GPU memory. As of now, D3D12 memory allocator is used.
    class MemoryAllocator
    {
      public:
        MemoryAllocator(ID3D12Device* const device, IDXGIAdapter* const adapter);
        ~MemoryAllocator() = default;

        MemoryAllocator(const MemoryAllocator& other) = delete;
        MemoryAllocator& operator=(const MemoryAllocator& other) = delete;

        MemoryAllocator(MemoryAllocator&& other) = delete;
        MemoryAllocator& operator=(MemoryAllocator&& other) = delete;

        // While both the CreateXResourceAllocation can be merged, it leads to a bit strange / awkward code, so
        // separating it for now.
        std::unique_ptr<Allocation> createBufferResourceAllocation(const BufferCreationDesc& bufferCreationDesc,
                                                                   const ResourceCreationDesc& resourceCreationDesc);

      private:
        wrl::ComPtr<D3D12MA::Allocator> m_allocator{};
        std::recursive_mutex m_resourceAllocationMutex{};
    };

} // namespace helios::gfx