#pragma once

#include "CommandQueue.hpp"

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
        explicit GraphicsDevice();
        ~GraphicsDevice();

        GraphicsDevice(const GraphicsDevice& other) = delete;
        GraphicsDevice& operator=(const GraphicsDevice& other) = delete;

        GraphicsDevice(GraphicsDevice&& other) = delete;
        GraphicsDevice& operator=(GraphicsDevice&& other) = delete;

      private:
        void initDeviceResources();
        void initSwapchainResources();

      public:
        static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

      private:
        wrl::ComPtr<ID3D12Debug3> m_debug{};
        wrl::ComPtr<IDXGIFactory6> m_factory{};
        wrl::ComPtr<IDXGIAdapter2> m_adapter{};

        wrl::ComPtr<ID3D12Device5> m_device{};
        wrl::ComPtr<ID3D12DebugDevice> m_debugDevice{};

        std::unique_ptr<CommandQueue> m_directCommandQueue{};

        std::array<FenceValues, FRAMES_IN_FLIGHT> m_fenceValues{};

        uint64_t m_currentFrameIndex{};
    };
} // namespace helios::gfx