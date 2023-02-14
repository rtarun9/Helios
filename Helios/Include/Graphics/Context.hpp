#pragma once

namespace helios::gfx
{
    // Base class for Context (i.e wrapper for command list and a command allocator). Provides batching of resource
    // barriers for optimal performance. It uses a ID3D12GraphicsCommandList and is can execute commands of any type
    // (copy, compute, graphics, etc).
    class Context
    {
      public:
        ID3D12GraphicsCommandList1* const getCommandList() const
        {
            return m_commandList.Get();
        }

        void addResourceBarrier(ID3D12Resource* const resource, const D3D12_RESOURCE_STATES previousState,
                                const D3D12_RESOURCE_STATES newState);
        void executeResourceBarriers();

        virtual void reset();

      protected:
        explicit Context() = default;
        virtual ~Context() = default;

        wrl::ComPtr<ID3D12GraphicsCommandList1> m_commandList{};
        wrl::ComPtr<ID3D12CommandAllocator> m_commandAllocator{};

        // Resource barriers are quite heavy, and executing them in a single call (batched) is very efficient.
        // The resource barriers are executed when the ExecuteResourceBarriers() call is invoked, which must happen
        // before command list is sent over to the device for execution, or be batched as much as possible.
        std::vector<CD3DX12_RESOURCE_BARRIER> m_resourceBarriers{};
    };
} // namespace helios::gfx