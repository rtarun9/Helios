#pragma once

#include "DescriptorHeap.hpp"

namespace helios::gfx
{
    struct BackBuffer
    {
        wrl::ComPtr<ID3D12Resource> backBufferResource;
        DescriptorHandle backBufferDescriptorHandle;

        ID3D12Resource* const GetResource()
        {
            return backBufferResource.Get();
        }

        std::wstring bufferName{};
    };
}