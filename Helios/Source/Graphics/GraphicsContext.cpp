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

        setDescriptorHeaps();

        throwIfFailed(m_commandList->Close());
    }

    void GraphicsContext::reset()
    {
        Context::reset();

        setDescriptorHeaps();
    }

    void GraphicsContext::setDescriptorHeaps() const
    {
        // As all graphics context's require to set the descriptor heap before hand, the user has option to set them
        // manually (for explicitness) or just let the constructor take care of this.
        const std::array<const DescriptorHeap* const, 2u> shaderVisibleDescriptorHeaps = {
            graphicsDevice.getCbvSrvUavDescriptorHeap(),
            graphicsDevice.getSamplerDescriptorHeap(),
        };

        std::vector<ID3D12DescriptorHeap*> descriptorHeaps{};
        descriptorHeaps.reserve(shaderVisibleDescriptorHeaps.size());
        for (const auto& heap : shaderVisibleDescriptorHeaps)
        {
            descriptorHeaps.emplace_back(heap->getDescriptorHeap());
        };

        m_commandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
    }

    void GraphicsContext::clearRenderTargetView(const Texture& renderTarget, std::span<const float, 4> color)
    {
        const auto rtvDescriptorHandle =
            graphicsDevice.getRtvDescriptorHeap()->getDescriptorHandleFromIndex(renderTarget.rtvIndex);

        m_commandList->ClearRenderTargetView(rtvDescriptorHandle.cpuDescriptorHandle, color.data(), 0u, nullptr);
    }

    void GraphicsContext::clearRenderTargetView(const Texture& renderTarget, const float color)
    {
        const auto rtvDescriptorHandle =
            graphicsDevice.getRtvDescriptorHeap()->getDescriptorHandleFromIndex(renderTarget.rtvIndex);

        m_commandList->ClearRenderTargetView(rtvDescriptorHandle.cpuDescriptorHandle, (FLOAT*)&color, 0u, nullptr);
    }

    void GraphicsContext::clearRenderTargetView(const std::span<const Texture> renderTargets,
                                                const std::span<const float, 4> color)
    {
        for (const uint32_t i : std::views::iota(0u, renderTargets.size()))
        {
            const auto rtvDescriptorHandle =
                graphicsDevice.getRtvDescriptorHeap()->getDescriptorHandleFromIndex(renderTargets[i].rtvIndex);
            m_commandList->ClearRenderTargetView(rtvDescriptorHandle.cpuDescriptorHandle, color.data(), 0u, nullptr);
        }
    }

    void GraphicsContext::clearDepthStencilView(const Texture& texture)
    {
        const auto dsvDescriptorHandle =
            graphicsDevice.getDsvDescriptorHeap()->getDescriptorHandleFromIndex(texture.dsvIndex);

        m_commandList->ClearDepthStencilView(dsvDescriptorHandle.cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 1u,
                                             0u, nullptr);
    }

    void GraphicsContext::setGraphicsPipelineState(const PipelineState& pipelineState) const
    {
        m_commandList->SetPipelineState(pipelineState.m_pipelineStateObject.Get());
    }

    void GraphicsContext::setGraphicsRootSignature(const PipelineState& pipelineState) const
    {
        m_commandList->SetGraphicsRootSignature(pipelineState.s_rootSignature.Get());
    }

    void GraphicsContext::setGraphicsRootSignatureAndPipeline(const PipelineState& pipelineState) const
    {
        m_commandList->SetGraphicsRootSignature(PipelineState::s_rootSignature.Get());
        m_commandList->SetPipelineState(pipelineState.m_pipelineStateObject.Get());
    }

    void GraphicsContext::setIndexBuffer(const Buffer& buffer) const
    {
        const D3D12_INDEX_BUFFER_VIEW indexBufferView = {
            .BufferLocation = buffer.allocation.resource->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(buffer.sizeInBytes),
            .Format = DXGI_FORMAT_R32_UINT,
        };

        m_commandList->IASetIndexBuffer(&indexBufferView);
    }

    void GraphicsContext::set32BitGraphicsConstants(const void* renderResources) const
    {
        m_commandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, renderResources, 0u);
    }

    void GraphicsContext::setComputeRootSignatureAndPipeline(const PipelineState& pipelineState) const
    {
        m_commandList->SetComputeRootSignature(PipelineState::s_rootSignature.Get());
        m_commandList->SetPipelineState(pipelineState.m_pipelineStateObject.Get());
    }

    void GraphicsContext::set32BitComputeConstants(const void* renderResources) const
    {
        m_commandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, renderResources, 0u);
    }

    void GraphicsContext::setViewport(const D3D12_VIEWPORT& viewport) const
    {
        static D3D12_RECT scissorRect{.left = 0u, .top = 0u, .right = LONG_MAX, .bottom = LONG_MAX};

        m_commandList->RSSetViewports(1u, &viewport);
        m_commandList->RSSetScissorRects(1u, &scissorRect);
    }

    // Specifies how the pipeline interprets vertex data bound to the input assembler stage.
    // i.e if topology type is POINTLIST, vertex data is interpreted as list of points.
    void GraphicsContext::setPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY primitiveTopology) const
    {
        m_commandList->IASetPrimitiveTopology(primitiveTopology);
    }

    void GraphicsContext::setRenderTarget(const Texture& renderTarget) const
    {
        const auto rtvDescriptorHandle =
            graphicsDevice.getRtvDescriptorHeap()->getDescriptorHandleFromIndex(renderTarget.rtvIndex);

        m_commandList->OMSetRenderTargets(1u, &rtvDescriptorHandle.cpuDescriptorHandle, FALSE, nullptr);
    }

    void GraphicsContext::setRenderTarget(const Texture& renderTarget, const Texture& depthStencilTexture) const
    {
        const auto rtvDescriptorHandle =
            graphicsDevice.getRtvDescriptorHeap()->getDescriptorHandleFromIndex(renderTarget.rtvIndex);

        const auto dsvDescriptorHandle =
            graphicsDevice.getDsvDescriptorHeap()->getDescriptorHandleFromIndex(depthStencilTexture.dsvIndex);

        m_commandList->OMSetRenderTargets(1u, &rtvDescriptorHandle.cpuDescriptorHandle, FALSE,
                                          &dsvDescriptorHandle.cpuDescriptorHandle);
    }

    void GraphicsContext::setRenderTarget(const std::span<const Texture> renderTargets,
                                          const Texture& depthStencilTexture) const
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvDescriptorHandles(renderTargets.size());
        for (const uint32_t i : std::views::iota(0u, renderTargets.size()))
        {
            rtvDescriptorHandles[i] = graphicsDevice.getRtvDescriptorHeap()
                                          ->getDescriptorHandleFromIndex(renderTargets[i].rtvIndex)
                                          .cpuDescriptorHandle;
        }

        const auto dsvDescriptorHandle =
            graphicsDevice.getDsvDescriptorHeap()->getDescriptorHandleFromIndex(depthStencilTexture.dsvIndex);

        m_commandList->OMSetRenderTargets(static_cast<uint32_t>(renderTargets.size()), rtvDescriptorHandles.data(),
                                          TRUE, &dsvDescriptorHandle.cpuDescriptorHandle);
    }

    void GraphicsContext::copyResource(ID3D12Resource* const source, ID3D12Resource* const destination) const
    {
        m_commandList->CopyResource(destination, source);
    }

    void GraphicsContext::drawInstanceIndexed(const uint32_t indicesCount, const uint32_t instanceCount) const
    {
        m_commandList->DrawIndexedInstanced(indicesCount, instanceCount, 0u, 0u, 0u);
    }

    void GraphicsContext::drawIndexed(const uint32_t indicesCount, const uint32_t instanceCount) const
    {
        m_commandList->DrawInstanced(indicesCount, instanceCount, 0u, 0u);
    }

    void GraphicsContext::dispatch(const uint32_t threadGroupDimX, const uint32_t threadGroupDimY,
                                   const uint32_t threadGroupDimZ)
    {
        m_commandList->Dispatch(threadGroupDimX, threadGroupDimY, threadGroupDimZ);
    }

} // namespace helios::gfx
