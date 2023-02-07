#include "Graphics/ComputeContext.hpp"

#include "Graphics/DescriptorHeap.hpp"
#include "Graphics/GraphicsDevice.hpp"

namespace helios::gfx
{
    ComputeContext::ComputeContext(GraphicsDevice* const device) : graphicsDevice(*device)
    {
        throwIfFailed(device->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                                  IID_PPV_ARGS(&m_commandAllocator)));
        throwIfFailed(device->getDevice()->CreateCommandList(
            0u, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

        // As all graphics context's require to set the descriptor heap before hand, the user has option to set them
        // manually (for explicitness) or just let the constructor take care of this.
        const std::array<const DescriptorHeap* const, 2u> descriptorHeaps = {
            device->getCbvSrvUavDescriptorHeap(),
            device->getSamplerDescriptorHeap(),
        };

        setDescriptorHeaps(descriptorHeaps);

        throwIfFailed(m_commandList->Close());
    }

    void ComputeContext::reset()
    {
        Context::reset();

        const std::array<const DescriptorHeap* const, 2u> descriptorHeaps = {
            graphicsDevice.getCbvSrvUavDescriptorHeap(),
            graphicsDevice.getSamplerDescriptorHeap(),
        };

        setDescriptorHeaps(descriptorHeaps);
    }

    void ComputeContext::setDescriptorHeaps(
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

   void ComputeContext::dispatch(const uint32_t threadGroupX, const uint32_t threadGroupY, const uint32_t threadGroupZ) const
    {
        m_commandList->Dispatch(threadGroupX, threadGroupY, threadGroupZ);
   }

    void ComputeContext::setComputeRootSignatureAndPipeline(const PipelineState& pipelineState) const
    {
        m_commandList->SetComputeRootSignature(PipelineState::s_rootSignature.Get());
        m_commandList->SetPipelineState(pipelineState.m_pipelineStateObject.Get());
    }

    void ComputeContext::set32BitComputeConstants(const void* renderResources) const
    {
        m_commandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, renderResources, 0u);
    }

} // namespace helios::gfx
