#include "Graphics/GraphicsDevice.hpp"

namespace helios::gfx
{
    GraphicsDevice::GraphicsDevice()
    {
        initDeviceResources();
    }

    GraphicsDevice::~GraphicsDevice()
    {
    }

    void GraphicsDevice::initDeviceResources()
    {
        // Enable the D3D12 debug layer in Debug build configurations.
        if constexpr (HELIOS_DEBUG_MODE)
        {
            throwIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug)));
            m_debug->EnableDebugLayer();
            m_debug->SetEnableGPUBasedValidation(true);
            m_debug->SetEnableSynchronizedCommandQueueValidation(true);
        }

        // Create the DXGI factory (can be used to enumerate adapters).

        uint32_t dxgiFactoryCreationFlags = 0u;
        if constexpr (HELIOS_DEBUG_MODE)
        {
            dxgiFactoryCreationFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        throwIfFailed(::CreateDXGIFactory2(dxgiFactoryCreationFlags, IID_PPV_ARGS(&m_factory)));

        // Select the adapter (in this case GPU with best performance).
        throwIfFailed(
            m_factory->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)));

        if (!m_adapter)
        {
            fatalError("Failed to find D3D12 compatible adapter");
        }

        // Display information about the selected adapter.
        DXGI_ADAPTER_DESC adapterDesc{};
        throwIfFailed(m_adapter->GetDesc(&adapterDesc));
        log(std::format(L"Chosen adapter : {}.", adapterDesc.Description));
        
        // Create D3D12 Device.
        throwIfFailed(::D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
        m_device->SetName(L"D3D12 Device");

        // Set breakpoints on certain debug messages in debug build configurations.
        if constexpr (HELIOS_DEBUG_MODE)
        {
            wrl::ComPtr<ID3D12InfoQueue> infoQueue;
            throwIfFailed(m_device.As(&infoQueue));

            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, false);

            // Get the debug device. It represents a graphics device for debugging, while the debug interface controls debug
            // settings and validates pipeline state.
            // Debug device can be used to check for reporting live objects / leaks.
            throwIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&m_debugDevice)));
        }

        // Create the command queue's.
        m_directCommandQueue =
            std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Direct Command Queue");
    }

    void GraphicsDevice::initSwapchainResources()
    {
    }
} // namespace helios::gfx