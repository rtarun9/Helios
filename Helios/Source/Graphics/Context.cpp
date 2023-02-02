#include "Graphics/Context.hpp"

namespace helios::gfx
{
    void Context::addResourceBarrier(ID3D12Resource* const resource, const D3D12_RESOURCE_STATES previousState,
                                     const D3D12_RESOURCE_STATES newState)
    {
        m_resourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, previousState, newState));
    }

    void Context::executeResourceBarriers()
    {
        m_commandList->ResourceBarrier(static_cast<UINT>(m_resourceBarriers.size()), m_resourceBarriers.data());
        m_resourceBarriers.clear();
    }
} // namespace helios::gfx