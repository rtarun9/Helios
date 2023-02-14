#pragma once

namespace helios::gfx
{
    // Holds a CPU and GPU descriptor handle.
    struct DescriptorHandle
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};

        uint32_t descriptorSize{};

        void offset()
        {
            cpuDescriptorHandle.ptr += descriptorSize;
            gpuDescriptorHandle.ptr += descriptorSize;
        }
    };

    // DescriptorHeap abstraction that has a 'current descriptor' that comes in handy while initializing resource
    // (Texture's, buffer's etc). Has methods to return index of current descriptor : most resource abstractions
    // (texture's, buffer's) etc store this index and use for bindless rendering.
    // DescriptorHeap is a contiguous linear allocation which stores descriptor's, which are tiny blocks of memory
    // describing a resource.
    class DescriptorHeap
    {
      public:
        explicit DescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
                                const uint32_t descriptorCount, const std::wstring_view descriptorHeapName);
        ~DescriptorHeap() = default;

        DescriptorHeap(const DescriptorHeap& other) = delete;
        DescriptorHeap& operator=(const DescriptorHeap& other) = delete;

        DescriptorHeap(DescriptorHeap&& other) = delete;
        DescriptorHeap& operator=(DescriptorHeap&& other) = delete;

        ID3D12DescriptorHeap* const getDescriptorHeap() const
        {
            return m_descriptorHeap.Get();
        }

        uint32_t getDescriptorSize() const
        {
            return m_descriptorSize;
        };

        DescriptorHandle getDescriptorHandleFromStart() const
        {
            return m_descriptorHandleFromHeapStart;
        };

        DescriptorHandle getCurrentDescriptorHandle() const
        {
            return m_currentDescriptorHandle;
        };

        DescriptorHandle getDescriptorHandleFromIndex(const uint32_t index) const;

        // Returns a index that can be used to directly index into a descriptor heap.
        [[nodiscard]] uint32_t getDescriptorIndex(const DescriptorHandle& descriptorHandle) const;
        [[nodiscard]] uint32_t getCurrentDescriptorIndex() const;

        // Used to offset a X_Handle passed into function.
        void offsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset = 1u) const;
        void offsetDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset = 1u) const;
        void offsetDescriptor(DescriptorHandle& handle, const uint32_t offset = 1u) const;

        void offsetCurrentHandle(const uint32_t offset = 1u);

      private:
        wrl::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap{};
        uint32_t m_descriptorSize{};

        DescriptorHandle m_descriptorHandleFromHeapStart{};
        DescriptorHandle m_currentDescriptorHandle{};
    };
} // namespace helios::gfx