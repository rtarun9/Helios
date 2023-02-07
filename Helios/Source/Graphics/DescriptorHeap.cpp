#include "Graphics/DescriptorHeap.hpp"

namespace helios::gfx
{
    DescriptorHeap::DescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
                                   const uint32_t descriptorCount, const std::wstring_view descriptorHeapName)
    {
        const D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags = (descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
                                                                 descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
                                                                    ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                                                                    : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        const D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {
            .Type = descriptorHeapType,
            .NumDescriptors = descriptorCount,
            .Flags = descriptorHeapFlags,
            .NodeMask = 0u,
        };

        throwIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap)));
        m_descriptorHeap->SetName(descriptorHeapName.data());

        m_descriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

        m_descriptorHandleFromHeapStart = {
            .cpuDescriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            .gpuDescriptorHandle = (descriptorHeapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                                       ? m_descriptorHeap->GetGPUDescriptorHandleForHeapStart()
                                       : CD3DX12_GPU_DESCRIPTOR_HANDLE{},
            .descriptorSize = m_descriptorSize,
        };

        m_currentDescriptorHandle = m_descriptorHandleFromHeapStart;
    }

    DescriptorHandle DescriptorHeap::getDescriptorHandleFromIndex(const uint32_t index) const
    {
        DescriptorHandle handle = getDescriptorHandleFromStart();
        offsetDescriptor(handle, index);

        return std::move(handle);
    }

    uint32_t DescriptorHeap::getDescriptorIndex(const DescriptorHandle& descriptorHandle) const
    {
        return static_cast<uint32_t>(
            (descriptorHandle.gpuDescriptorHandle.ptr - m_descriptorHandleFromHeapStart.gpuDescriptorHandle.ptr) /
            m_descriptorSize);
    }

    uint32_t DescriptorHeap::getCurrentDescriptorIndex() const
    {
        return getDescriptorIndex(m_currentDescriptorHandle);
    }

    void DescriptorHeap::offsetDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset) const
    {
        handle.ptr += m_descriptorSize * static_cast<unsigned long long>(offset);
    }

    void DescriptorHeap::offsetDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const uint32_t offset) const
    {
        handle.ptr += m_descriptorSize * static_cast<unsigned long long>(offset);
    }

    void DescriptorHeap::offsetDescriptor(DescriptorHandle& descriptorHandle, const uint32_t offset) const
    {
        descriptorHandle.cpuDescriptorHandle.ptr += m_descriptorSize * static_cast<unsigned long long>(offset);
        descriptorHandle.gpuDescriptorHandle.ptr += m_descriptorSize * static_cast<unsigned long long>(offset);
    }

    void DescriptorHeap::offsetCurrentHandle(const uint32_t offset)
    {
        offsetDescriptor(m_currentDescriptorHandle, offset);
    }
} // namespace helios::gfx