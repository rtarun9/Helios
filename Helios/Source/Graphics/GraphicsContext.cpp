#include "Graphics/GraphicsContext.hpp"
#include "Graphics/DescriptorHeap.hpp"
#include "Graphics/GraphicsDevice.hpp"

namespace helios::gfx
{
    GraphicsContext::GraphicsContext(GraphicsDevice* const device) : graphicsDevice(*device)
    {
        throwIfFailed(device->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                                  IID_PPV_ARGS(&m_commandAllocator)));
        throwIfFailed(device->getDevice()->CreateCommandList(
            0u, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

        // As all graphics context's require to set the descriptor heap before hand, the user has option to set them
        // manually (for explicitness) or just let the constructor take care of this.
        const std::array<const DescriptorHeap* const, 1u> descriptorHeaps = {
            device->getCbvSrvUavDescriptorHeap(),
        };

        setDescriptorHeaps(descriptorHeaps);

        throwIfFailed(m_commandList->Close());
    }

    void GraphicsContext::setDescriptorHeaps(
        const std::span<const DescriptorHeap* const> shaderVisibleDescriptorHeaps) const
    {
        std::vector<ID3D12DescriptorHeap*> descriptorHeaps{};
        descriptorHeaps.reserve(shaderVisibleDescriptorHeaps.size());
        for (const auto& heap : shaderVisibleDescriptorHeaps)
        {
            descriptorHeaps.emplace_back(heap->getDescriptorHeap());
        };

        m_commandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
    }

    void GraphicsContext::clearRenderTargetView(BackBuffer& backBuffer, std::span<const float, 4> color)
    {
        m_commandList->ClearRenderTargetView(backBuffer.backBufferDescriptorHandle.cpuDescriptorHandle, color.data(),
                                             0u, nullptr);
    }
} // namespace helios::gfx
