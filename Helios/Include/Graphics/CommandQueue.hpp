#pragma once

#include "Context.hpp"

namespace helios::gfx
{
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

        void executeContext(const std::span<const Context> contexts);

        // Synchronization related functions.
        bool isFenceComplete(const uint64_t fenceValue) const;
        uint64_t signal(const uint64_t fenceValue) const;
        void waitForFenceValue(const uint64_t fenceValue) const;
        uint64_t getCurrentFenceValue() const;

        void flush(const uint64_t fenceValue);

      private:
        wrl::ComPtr<ID3D12CommandQueue> m_commandQueue{};
        wrl::ComPtr<ID3D12Fence> m_fence{};
    };
} // namespace helios::gfx