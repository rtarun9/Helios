#include "Graphics/CommandQueue.hpp"
#include "Graphics/Context.hpp"

namespace helios::gfx
{
    CommandQueue::CommandQueue(ID3D12Device5* const device, const D3D12_COMMAND_LIST_TYPE commandListType,
                               const std::wstring_view name)
    {
        // Create the command queue based on list type.
        const D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
            .Type = commandListType,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0u,
        };

        throwIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));
        m_commandQueue->SetName(name.data());

        // Create the fence (used for synchronization of CPU and GPU).
        throwIfFailed(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fence->SetName(name.data());
    }

    void CommandQueue::executeContext(const std::span<const Context* const> contexts)
    {
        std::vector<ID3D12CommandList*> commandLists{};
        commandLists.reserve(contexts.size());

        for (auto& context : contexts)
        {
            throwIfFailed(context->getCommandList()->Close());
            commandLists.emplace_back(context->getCommandList());
        }

        m_commandQueue->ExecuteCommandLists(static_cast<UINT>(contexts.size()), commandLists.data());
    }

    bool CommandQueue::isFenceComplete(const uint64_t fenceValue) const
    {
        return m_fence->GetCompletedValue() >= fenceValue;
    }

    uint64_t CommandQueue::signal()
    {
        m_monotonicallyIncreasingFenceValue++;
        throwIfFailed(m_commandQueue->Signal(m_fence.Get(), m_monotonicallyIncreasingFenceValue));

        return m_monotonicallyIncreasingFenceValue;
    }

    void CommandQueue::waitForFenceValue(const uint64_t fenceValue) const
    {
        if (!isFenceComplete(fenceValue))
        {
            throwIfFailed(m_fence->SetEventOnCompletion(fenceValue, nullptr));
        }
    }

    void CommandQueue::flush()
    {
        const uint64_t fenceValueToWaitFor = signal();
        waitForFenceValue(fenceValueToWaitFor);
    }
} // namespace helios::gfx