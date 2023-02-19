#include "Graphics/Context.hpp"

namespace helios::gfx
{
    void Context::addResourceBarrier(ID3D12Resource* const resource, const D3D12_RESOURCE_STATES previousState,
                                     const D3D12_RESOURCE_STATES newState)
    {
        m_resourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState));
    }

    void Context::addResourceBarrier(ID3D12Resource* const resource)
    {
        m_resourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(resource));
    }

    void Context::addResourceBarrier(const CD3DX12_RESOURCE_BARRIER& resourceBarrier)
    {
        m_resourceBarriers.emplace_back(resourceBarrier);
    }

    void Context::executeResourceBarriers()
    {
        m_commandList->ResourceBarrier(static_cast<UINT>(m_resourceBarriers.size()), m_resourceBarriers.data());
        m_resourceBarriers.clear();
    }

    void Context::reset()
    {
        throwIfFailed(m_commandAllocator->Reset());
        throwIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
    }
} // namespace helios::gfx