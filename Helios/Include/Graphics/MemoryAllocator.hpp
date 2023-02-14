#pragma once

#include "Resources.hpp"

namespace helios::gfx
{
    // Memory allocator handles allocation of GPU memory. As of now, D3D12 memory allocator is used.
    // D3D12MA is used rather than creation of committed resources. This is because Placed resources (which D3D12MA may
    // allocate) can be more memory efficient. Also, a lot of issues such as fragmentation are handled automatically by
    // D3D12MA. A 'Allocation' represents a GPU allocation (i.e represents a heap allocated from the GPU memory (based
    // on the Heap Type, that is either on GPU only memory or on the shared CPU / GPU memory)).
    class MemoryAllocator
    {
      public:
        explicit MemoryAllocator(ID3D12Device* const device, IDXGIAdapter* const adapter);
        ~MemoryAllocator() = default;

        MemoryAllocator(const MemoryAllocator& other) = delete;
        MemoryAllocator& operator=(const MemoryAllocator& other) = delete;

        MemoryAllocator(MemoryAllocator&& other) = delete;
        MemoryAllocator& operator=(MemoryAllocator&& other) = delete;

        // While both the CreateXResourceAllocation can be merged, it leads to a bit strange / awkward code, so
        // separating it for now.
        // The Allocation represents a ID3D12Resource and a D3D12MA::Allocation.
        [[nodiscard]] Allocation createBufferResourceAllocation(const BufferCreationDesc& bufferCreationDesc,
                                                                const ResourceCreationDesc& resourceCreationDesc);

        [[nodiscard]] Allocation createTextureResourceAllocation(const TextureCreationDesc& textureCreationDesc);

      private:
        wrl::ComPtr<D3D12MA::Allocator> m_allocator{};
        std::recursive_mutex m_resourceAllocationMutex{};
    };

} // namespace helios::gfx