#include "Graphics/CommandQueue.hpp"

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

    void executeContext(const std::span<const Context> contexts)
    {
        // TODO(rtarun9) : Fill in this function.
        return;
    }

    bool CommandQueue::isFenceComplete(const uint64_t fenceValue) const
    {
        return m_fence->GetCompletedValue() >= fenceValue;
    }

    uint64_t CommandQueue::signal(const uint64_t fenceValue) const
    {
        throwIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceValue));

        return fenceValue + 1u;
    }

    void CommandQueue::waitForFenceValue(const uint64_t fenceValue) const
    {
        if (!isFenceComplete(fenceValue))
        {
            throwIfFailed(m_fence->SetEventOnCompletion(fenceValue, nullptr));
            ::WaitForSingleObject(nullptr, INFINITE);
        }
    }

    uint64_t CommandQueue::getCurrentFenceValue() const
    {
        return m_fence->GetCompletedValue();
    }

    void CommandQueue::flush(const uint64_t fenceValue)
    {
        const uint64_t fenceValueToWaitFor = signal(fenceValue);
        waitForFenceValue(fenceValueToWaitFor);
    }
} // namespace helios::gfx