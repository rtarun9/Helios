#include "Graphics/GraphicsDevice.hpp"

namespace helios::gfx
{
    GraphicsDevice::GraphicsDevice(const uint32_t windowWidth, const uint32_t windowHeight,
                                   const DXGI_FORMAT swapchainFormat, const HWND windowHandle)
    {
        initDeviceResources();
        initSwapchainResources(windowWidth, windowHeight, swapchainFormat, windowHandle);

        m_isInitialized = true;
    }

    GraphicsDevice::~GraphicsDevice()
    {
        m_directCommandQueue->flush();
    }

    void GraphicsDevice::beginFrame()
    {
        m_perFrameGraphicsContexts[m_currentFrameIndex]->reset();
    }

    void GraphicsDevice::present()
    {
        throwIfFailed(m_swapchain->Present(1u, 0u));
    }

    void GraphicsDevice::endFrame()
    {
        m_fenceValues[m_currentFrameIndex].directQueueFenceValue = m_directCommandQueue->signal();

        m_currentFrameIndex = m_swapchain->GetCurrentBackBufferIndex();

        m_directCommandQueue->waitForFenceValue(m_fenceValues[m_currentFrameIndex].directQueueFenceValue);
    }

    void GraphicsDevice::initDeviceResources()
    {
        initD3D12Core();
        initCommandQueues();
        initDescriptorHeaps();

        // Create graphics contexts (one per frame in flight).
        for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
        {
            m_perFrameGraphicsContexts[i] = std::make_unique<GraphicsContext>(this);
        }
    }

    void GraphicsDevice::initSwapchainResources(const uint32_t windowWidth, const uint32_t windowHeight,
                                                const DXGI_FORMAT swapchainFormat, const HWND windowHandle)
    {
        // Check if the tearing feature is supported.
        BOOL tearingSupported = TRUE;
        if (FAILED(m_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupported,
                                                  sizeof(tearingSupported))))
        {
            tearingSupported = FALSE;
        }

        m_tearingSupported = tearingSupported;

        const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = windowWidth,
            .Height = windowHeight,
            .Format = swapchainFormat,
            .Stereo = FALSE,
            .SampleDesc{
                .Count = 1,
                .Quality = 0,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = FRAMES_IN_FLIGHT,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
            .Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u,
        };

        wrl::ComPtr<IDXGISwapChain1> swapChain1;
        throwIfFailed(m_factory->CreateSwapChainForHwnd(m_directCommandQueue->getCommandQueue(), windowHandle,
                                                        &swapChainDesc, nullptr, nullptr, &swapChain1));

        // Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
        throwIfFailed(m_factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));

        throwIfFailed(swapChain1.As(&m_swapchain));

        m_currentFrameIndex = m_swapchain->GetCurrentBackBufferIndex();

        createBackBufferRTVs();
    }

    void GraphicsDevice::initD3D12Core()
    {
        // Enable the D3D12 debug layer in Debug build configurations.
        if constexpr (HELIOS_DEBUG_MODE)
        {
            throwIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug)));
            m_debug->EnableDebugLayer();
            m_debug->SetEnableGPUBasedValidation(true);
            m_debug->SetEnableSynchronizedCommandQueueValidation(true);
        }

        // Create the DXGI factory (can be used to enumerate adapters and other information on the graphics subsystem
        // (memory, adapters, etc)).
        uint32_t dxgiFactoryCreationFlags{};
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
            wrl::ComPtr<ID3D12InfoQueue> infoQueue{};
            throwIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue)));

            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, false);

            // Get the debug device. It represents a graphics device for debugging, while the debug interface controls
            // debug settings and validates pipeline state. Debug device can be used to check for reporting live objects
            // and leaks.
            throwIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&m_debugDevice)));
        }
    }

    void GraphicsDevice::initCommandQueues()
    {
        // Create the command queue's.
        m_directCommandQueue =
            std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Direct Command Queue");
    }

    void GraphicsDevice::initDescriptorHeaps()
    {
        // Create descriptor heaps.
        m_cbvSrvUavDescriptorHeap = std::make_unique<DescriptorHeap>(
            m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100u, L"CBV SRV UAV Descriptor Heap");

        m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 10u,
                                                               L"RTV Descriptor Heap");
    }

    void GraphicsDevice::createBackBufferRTVs()
    {
        m_currentFrameIndex = m_swapchain->GetCurrentBackBufferIndex();

        DescriptorHandle rtvHandle = m_rtvDescriptorHeap->getDescriptorHandleFromStart();

        // Create Backbuffer render target views.
        for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
        {
            wrl::ComPtr<ID3D12Resource> backBuffer{};
            throwIfFailed(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

            m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle.cpuDescriptorHandle);

            m_backBuffers[i].backBufferResource = backBuffer;
            m_backBuffers[i].backBufferResource->SetName(L"SwapChain BackBuffer");

            m_backBuffers[i].backBufferDescriptorHandle = rtvHandle;

            m_rtvDescriptorHeap->offsetDescriptor(rtvHandle);
        }

        if (!m_isInitialized)
        {
            m_rtvDescriptorHeap->offsetCurrentHandle(FRAMES_IN_FLIGHT);
        }
    }
} // namespace helios::gfx