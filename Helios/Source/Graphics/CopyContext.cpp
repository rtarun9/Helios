#include "Graphics/CopyContext.hpp"
#include "Graphics/DescriptorHeap.hpp"
#include "Graphics/GraphicsDevice.hpp"

namespace helios::gfx
{
    CopyContext::CopyContext(GraphicsDevice* const device) : graphicsDevice(*device)
    {
        throwIfFailed(device->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
                                                                  IID_PPV_ARGS(&m_commandAllocator)));

        throwIfFailed(device->getDevice()->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_COPY, m_commandAllocator.Get(),
                                                             nullptr, IID_PPV_ARGS(&m_commandList)));

        throwIfFailed(m_commandList->Close());
    }
} // namespace helios::gfx
