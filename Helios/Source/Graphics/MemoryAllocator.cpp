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

    Allocation MemoryAllocator::createBufferResourceAllocation(const BufferCreationDesc& bufferCreationDesc,
                                                               const ResourceCreationDesc& resourceCreationDesc)
    {
        Allocation allocation{};

        D3D12_RESOURCE_STATES resourceState{};
        D3D12_HEAP_TYPE heapType{};
        bool isCpuVisible = false;

        switch (bufferCreationDesc.usage)
        {
        case BufferUsage::UploadBuffer:
        case BufferUsage::ConstantBuffer: {
            // GenericRead implies readable data from the GPU memory. Required resourceState for upload heaps.
            // UploadHeap : CPU writable access, GPU readable access.
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

        std::lock_guard<std::recursive_mutex> resourceAllocationLockGuard(m_resourceAllocationMutex);

        throwIfFailed(m_allocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState,
                                                  nullptr, &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));

        if (isCpuVisible)
        {
            // Give the mapped pointer some value to hold.
            allocation.mappedPointer = nullptr;
            throwIfFailed(allocation.resource->Map(0u, nullptr, &allocation.mappedPointer.value()));
        }

        allocation.resource->SetName(bufferCreationDesc.name.data());
        allocation.allocation->SetResource(allocation.resource.Get());

        return allocation;
    }

    Allocation MemoryAllocator::createTextureResourceAllocation(const TextureCreationDesc& textureCreationDesc)
    {
        Allocation allocation{};

        DXGI_FORMAT format = textureCreationDesc.format;
        DXGI_FORMAT dsFormat{};

        switch (textureCreationDesc.format)
        {
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_TYPELESS: {
            dsFormat = DXGI_FORMAT_D32_FLOAT;
            format = DXGI_FORMAT_R32_FLOAT;
        }
        break;
        }

        ResourceCreationDesc resourceCreationDesc = {
            .resourceDesc =
                {
                    .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                    .Alignment = 0u,
                    .Width = textureCreationDesc.width,
                    .Height = textureCreationDesc.height,
                    .DepthOrArraySize = static_cast<UINT16>(textureCreationDesc.depthOrArraySize),
                    .MipLevels = static_cast<UINT16>(textureCreationDesc.mipLevels),
                    .Format = format,
                    .SampleDesc =
                        {
                            .Count = 1u,
                            .Quality = 0u,
                        },
                    .Flags = D3D12_RESOURCE_FLAG_NONE,
                },
        };

        // Clamp the mip level.
        if (resourceCreationDesc.resourceDesc.MipLevels >= resourceCreationDesc.resourceDesc.Width)
        {
            resourceCreationDesc.resourceDesc.MipLevels =
                static_cast<UINT16>(resourceCreationDesc.resourceDesc.Width - 1);
        }

        if (resourceCreationDesc.resourceDesc.MipLevels >= resourceCreationDesc.resourceDesc.Height)
        {
            resourceCreationDesc.resourceDesc.MipLevels =
                static_cast<UINT16>(resourceCreationDesc.resourceDesc.Height - 1);
        }

        const uint32_t mipLevels = resourceCreationDesc.resourceDesc.MipLevels;

        // As UpdateSubresources function is used to copy data from upload 'Buffer' to a 'Texture', we do not have to
        // care about the resource state or heap type. They can just be STATE_COMMON and HEAP_DEFAULT respectively.
        // Depth Stencil textures are an exception as resourceState will initially be set to DEPTH_WRITE.
        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
        constexpr D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12MA::ALLOCATION_DESC allocationDesc = {
            .HeapType = heapType,
        };

        // In the case of large resource (such as a render target, etc), its better for the resource to be a committed
        // resource and have a heap to itself.
        switch (textureCreationDesc.usage)
        {
        case TextureUsage::DepthStencil: {
            resourceCreationDesc.resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            resourceCreationDesc.resourceDesc.Format = dsFormat;
            allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
            resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        break;

        case TextureUsage::RenderTarget: {
            resourceCreationDesc.resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
            allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
            resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }
        break;

        // Note : All resource loaded from path must be able to be used by UAVs.
        case TextureUsage::TextureFromPath:
        case TextureUsage::TextureFromData:
        case TextureUsage::HDRTextureFromPath:
        case TextureUsage::CubeMap:
        case TextureUsage::UAVTexture: {
            resourceCreationDesc.resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        break;
        };

        std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{};

        if (textureCreationDesc.usage == TextureUsage::RenderTarget)
        {
            optimizedClearValue = {
                .Format = format,
                .Color =
                    {
                        0.0f,
                        0.0f,
                        0.0f,
                        1.0f,
                    },
            };
        }
        else if (textureCreationDesc.usage == TextureUsage::DepthStencil)
        {
            constexpr D3D12_DEPTH_STENCIL_VALUE dsValue = {
                .Depth = 1.0f,
                .Stencil = 1u,
            };

            optimizedClearValue = {
                .Format = dsFormat,
                .DepthStencil = dsValue,
            };
        }

        std::lock_guard<std::recursive_mutex> resourceAllocationLockGuard(m_resourceAllocationMutex);

        if (textureCreationDesc.optionalInitialState != D3D12_RESOURCE_STATE_COMMON)
        {
            resourceState = textureCreationDesc.optionalInitialState;
        }

        throwIfFailed(
            m_allocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState,
                                        optimizedClearValue.has_value() ? &optimizedClearValue.value() : nullptr,
                                        &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));

        allocation.resource->SetName(textureCreationDesc.name.data());
        allocation.allocation->SetResource(allocation.resource.Get());

        return allocation;
    }
} // namespace helios::gfx