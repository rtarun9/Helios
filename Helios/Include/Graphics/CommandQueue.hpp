#pragma once

namespace helios::gfx
{
    class Context;

    // Command queues are the execution port for GPU's.
    // We can submit command list's to them, and the recorded GPU commands will begin execution.
    // Synchronization in D3D12 must be done explicitly, and all sync primitives reside within the CommandQueue
    // abstraction.
    class CommandQueue
    {
      public:
        explicit CommandQueue(ID3D12Device5* const device, const D3D12_COMMAND_LIST_TYPE commandListType,
                              const std::wstring_view name);
        ~CommandQueue() = default;

        CommandQueue(const CommandQueue& other) = delete;
        CommandQueue& operator=(const CommandQueue& other) = delete;

        CommandQueue(CommandQueue&& other) = delete;
        CommandQueue& operator=(CommandQueue&& other) = delete;

        ID3D12CommandQueue* const getCommandQueue() const
        {
            return m_commandQueue.Get();
        }

        uint64_t getCurrentCompletedFenceValue() const
        {
            return m_fence->GetCompletedValue();
        }

        // The user / engine must take care that a context passed into the execute function can be used in the future 
        // (i.e proper synchronization must occur).
        // For now, each context consist of a command list and command allocator, and there is one command list per frame in flight (for direct command queue).
        void executeContext(const std::span<const Context* const> contexts);

      public:
        // Synchronization related functions.
        bool isFenceComplete(const uint64_t fenceValue) const;
        uint64_t signal();
        void waitForFenceValue(const uint64_t fenceValue) const;

        void flush();

      private:
        wrl::ComPtr<ID3D12CommandQueue> m_commandQueue{};
        wrl::ComPtr<ID3D12Fence> m_fence{};

        uint64_t m_monotonicallyIncreasingFenceValue{};
    };
} // namespace helios::gfx