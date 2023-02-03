#pragma once

#include "CommandQueue.hpp"
#include "DescriptorHeap.hpp"
#include "GraphicsContext.hpp"
#include "Resources.hpp"

namespace helios::gfx
{
    struct FenceValues
    {
        uint64_t directQueueFenceValue{};
    };

    // Abstraction for creating / destroying various graphics resources.
    // Encapsulates most renderer resources / objects in use : the swap chain, descriptor heaps, command queue's, etc.
    class GraphicsDevice
    {
      public:
        explicit GraphicsDevice(const uint32_t windowWidth, const uint32_t windowHeight,
                                const DXGI_FORMAT swapchainFormat, const HWND windowHandle);
        ~GraphicsDevice();

        GraphicsDevice(const GraphicsDevice& other) = delete;
        GraphicsDevice& operator=(const GraphicsDevice& other) = delete;

        GraphicsDevice(GraphicsDevice&& other) = delete;
        GraphicsDevice& operator=(GraphicsDevice&& other) = delete;

        void beginFrame();
        void present();
        void endFrame();

      public:
        CommandQueue* const getDirectCommandQueue() const
        {
            return m_directCommandQueue.get();
        }

        ID3D12Device5* const getDevice() const
        {
            return m_device.Get();
        }

        DescriptorHeap* const getCbvSrvUavDescriptorHeap() const
        {
            return m_cbvSrvUavDescriptorHeap.get();
        }

        DescriptorHeap* const getSamplerDescriptorHeap() const
        {
            return m_samplerDescriptorHeap.get();
        }

        std::unique_ptr<GraphicsContext>& getCurrentGraphicsContext()
        {
            return m_perFrameGraphicsContexts[m_currentFrameIndex];
        }

        BackBuffer& getCurrentBackBuffer()
        {
            return m_backBuffers[m_currentFrameIndex];
        }

      private:
        void initDeviceResources();
        void initSwapchainResources(const uint32_t windowWidth, const uint32_t windowHeight,
                                    const DXGI_FORMAT swapchainFormat, const HWND windowHandle);

      private:
        void initD3D12Core();
        void initCommandQueues();
        void initDescriptorHeaps();
        void initPerFrameContexts();

        void createBackBufferRTVs();

      public:
        static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

      private:
        wrl::ComPtr<IDXGIFactory6> m_factory{};
        wrl::ComPtr<ID3D12Debug3> m_debug{};
        wrl::ComPtr<ID3D12DebugDevice> m_debugDevice{};

        wrl::ComPtr<IDXGIAdapter2> m_adapter{};
        wrl::ComPtr<ID3D12Device5> m_device{};
        wrl::ComPtr<IDXGISwapChain4> m_swapchain{};

        std::unique_ptr<CommandQueue> m_directCommandQueue{};

        std::array<FenceValues, FRAMES_IN_FLIGHT> m_fenceValues{};
        std::array<BackBuffer, FRAMES_IN_FLIGHT> m_backBuffers{};
        uint64_t m_currentFrameIndex{};

        bool m_tearingSupported{false};

        std::unique_ptr<DescriptorHeap> m_rtvDescriptorHeap{};
        std::unique_ptr<DescriptorHeap> m_cbvSrvUavDescriptorHeap{};
        std::unique_ptr<DescriptorHeap> m_samplerDescriptorHeap{};

        std::array<std::unique_ptr<GraphicsContext>, FRAMES_IN_FLIGHT> m_perFrameGraphicsContexts{};

        bool m_isInitialized{false};
    };
} // namespace helios::gfx