#pragma once

#include "CommandQueue.hpp"
#include "CopyContext.hpp"
#include "DescriptorHeap.hpp"
#include "GraphicsContext.hpp"
#include "MemoryAllocator.hpp"
#include "MipMapGenerator.hpp"
#include "PipelineState.hpp"
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

      public:
        CommandQueue* const getDirectCommandQueue() const
        {
            return m_directCommandQueue.get();
        }

        CommandQueue* const getComputeCommandQueue() const
        {
            return m_computeCommandQueue.get();
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

        DescriptorHeap* const getDsvDescriptorHeap() const
        {
            return m_dsvDescriptorHeap.get();
        }

        DescriptorHeap* const getRtvDescriptorHeap() const
        {
            return m_rtvDescriptorHeap.get();
        }

        MipMapGenerator* const getMipMapGenerator() const
        {
            return m_mipMapGenerator.get();
        }

        DXGI_FORMAT getSwapchainBackBufferFormat() const
        {
            return m_swapchainBackBufferFormat;
        }

        [[nodiscard]] std::unique_ptr<GraphicsContext>& getCurrentGraphicsContext()
        {
            return m_perFrameGraphicsContexts[m_currentFrameIndex];
        }

        [[nodiscard]] std::unique_ptr<CopyContext>& getCopyContext()
        {
            return m_copyContext;
        }


        [[nodiscard]] Texture& getCurrentBackBuffer()
        {
            return m_backBuffers[m_currentFrameIndex];
        }

        [[nodiscard]] std::unique_ptr<ComputeContext> getComputeContext();
        void executeAndFlushComputeContext(std::unique_ptr<ComputeContext>&& computeContext);


        // Resets the current context (i.e the command list and the allocator).
        void beginFrame();
        void present();

        // Signals the direct command queue, and also waits for execution of the commands for next frame.
        void endFrame();

        void resizeWindow(const uint32_t windowWidth, const uint32_t windowHeight);

        // Creates a GPU Buffer. If some data is passed in, it will recursively call itself to create a upload buffer (a
        // buffer with CPU write and GPU read access and placed in a UploadHead). Then, a copy command is issued so that
        // the final Buffer returned by the function has the required data and is in exclusive GPU only memory.
        template <typename T>
        [[nodiscard]] Buffer createBuffer(const BufferCreationDesc& bufferCreationDesc,
                                          const std::span<const T> data = {}) const;

        // Creates a Texture that resides on GPU memory. The same Texture abstraction is used for render targets, depth
        // stencil texture, etc. If data is non-null, stb_image will be used to load texture (HDR and non HDR textures
        // supported). In that case, a upload buffer will be created, after which a copy command is issued so that
        // finally the texture on GPU only memory has the required data.
        [[nodiscard]] Texture createTexture(const TextureCreationDesc& textureCreationDesc,
                                            const void* data = nullptr) const;

        // Create a sampler (which is just an index into the sampler descriptor heap).
        // Do note that unlike createCbv/Srv/Uav this is placed in public access and not with the other create
        // function's which return indices into respective descriptor heaps. This is because we are not creating a
        // 'View' into the sampler. Samplers have no views, and the ID3D12Device function call itself is named
        // 'CreateSampler', and not 'Create Sampler View'.
        [[nodiscard]] Sampler createSampler(const SamplerCreationDesc& samplerCreationDesc) const;

        [[nodiscard]] PipelineState createPipelineState(
            const GraphicsPipelineStateCreationDesc& graphicsPipelineStateCreationDesc) const;

        [[nodiscard]] PipelineState createPipelineState(
            const ComputePipelineStateCreationDesc& computePipelineStateCreationDesc) const;

      private:
        void initDeviceResources();
        void initSwapchainResources(const uint32_t windowWidth, const uint32_t windowHeight);

        void initD3D12Core();
        void initCommandQueues();
        void initDescriptorHeaps();
        void initMemoryAllocator();
        void initContexts();
        void initBindlessRootSignature();
        void initMipMapGenerator();

        void createBackBufferRTVs();

        [[nodiscard]] uint32_t createCbv(const CbvCreationDesc& cbvCreationDesc) const;
        [[nodiscard]] uint32_t createSrv(const SrvCreationDesc& srvCreationDesc, ID3D12Resource* const resource) const;
        [[nodiscard]] uint32_t createUav(const UavCreationDesc& uavCreationDesc, ID3D12Resource* const resource) const;
        [[nodiscard]] uint32_t createRtv(const RtvCreationDesc& rtvCreationDesc, ID3D12Resource* const resource) const;
        [[nodiscard]] uint32_t createDsv(const DsvCreationDesc& dsvCreationDesc, ID3D12Resource* const resource) const;

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
        std::unique_ptr<CommandQueue> m_copyCommandQueue{};
        std::unique_ptr<CommandQueue> m_computeCommandQueue{};

        std::array<std::unique_ptr<GraphicsContext>, FRAMES_IN_FLIGHT> m_perFrameGraphicsContexts{};
        std::unique_ptr<CopyContext> m_copyContext{};
        std::queue<std::unique_ptr<ComputeContext>> m_computeContextQueue{};

        std::array<FenceValues, FRAMES_IN_FLIGHT> m_fenceValues{};
        std::array<Texture, FRAMES_IN_FLIGHT> m_backBuffers{};
        uint64_t m_currentFrameIndex{};

        DXGI_FORMAT m_swapchainBackBufferFormat{};
        HWND m_windowHandle{};

        bool m_tearingSupported{false};

        std::unique_ptr<DescriptorHeap> m_rtvDescriptorHeap{};
        std::unique_ptr<DescriptorHeap> m_dsvDescriptorHeap{};
        std::unique_ptr<DescriptorHeap> m_cbvSrvUavDescriptorHeap{};
        std::unique_ptr<DescriptorHeap> m_samplerDescriptorHeap{};

        std::unique_ptr<MemoryAllocator> m_memoryAllocator{};
        std::unique_ptr<MipMapGenerator> m_mipMapGenerator{};

        mutable std::recursive_mutex m_resourceMutex{};
        
        bool m_isInitialized{false};

        friend class MipMapGenerator;
    };

    template <typename T>
    Buffer GraphicsDevice::createBuffer(const BufferCreationDesc& bufferCreationDesc, std::span<const T> data) const
    {
        Buffer buffer{};

        // If data.size() == 0, it means that the data to fill the buffer will be passed later on (via the Update
        // functions).
        const uint32_t numberComponents = data.size() == 0 ? 1 : static_cast<uint32_t>(data.size());

        buffer.sizeInBytes = numberComponents * sizeof(T);

        const ResourceCreationDesc resourceCreationDesc =
            ResourceCreationDesc::createBufferResourceCreationDesc(buffer.sizeInBytes);

        buffer.allocation = m_memoryAllocator->createBufferResourceAllocation(bufferCreationDesc, resourceCreationDesc);

        std::scoped_lock<std::recursive_mutex> resourceLockGuard(m_resourceMutex);

        // Currently, not using a backing storage for upload context's and such. Simply using D3D12MA to create a upload
        // buffer, copy the data onto the upload buffer, and then copy data from upload buffer -> GPU only buffer.
        if (data.data())
        {
            // Create upload buffer.
            const BufferCreationDesc uploadBufferCreationDesc = {
                .usage = BufferUsage::UploadBuffer,
                .name = L"Upload buffer - " + std::wstring(bufferCreationDesc.name),
            };

            Allocation uploadAllocation =
                m_memoryAllocator->createBufferResourceAllocation(uploadBufferCreationDesc, resourceCreationDesc);

            uploadAllocation.update(data.data(), buffer.sizeInBytes);

            m_copyContext->reset();

            // Get a copy command and list and execute copy resource functions on the command queue.
            m_copyContext->getCommandList()->CopyResource(buffer.allocation.resource.Get(),
                                                          uploadAllocation.resource.Get());

            const std::array<helios::gfx::Context* const, 1u> contexts = {
                m_copyContext.get(),
            };

            m_copyCommandQueue->executeContext(contexts);
            m_copyCommandQueue->flush();

            uploadAllocation.reset();
        }

        // Create relevant descriptor's.
        if (bufferCreationDesc.usage == BufferUsage::StructuredBuffer)
        {
            const SrvCreationDesc srvCreationDesc = {
                .srvDesc =
                    {
                        .Format = DXGI_FORMAT_UNKNOWN,
                        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                        .Buffer =
                            {
                                .FirstElement = 0u,
                                .NumElements = static_cast<UINT>(data.size()),
                                .StructureByteStride = static_cast<UINT>(sizeof(T)),
                            },
                    },
            };

            buffer.srvIndex = createSrv(srvCreationDesc, buffer.allocation.resource.Get());
        }

        else if (bufferCreationDesc.usage == BufferUsage::ConstantBuffer)
        {
            const CbvCreationDesc cbvCreationDesc = {
                .cbvDesc =
                    {
                        .BufferLocation = buffer.allocation.resource->GetGPUVirtualAddress(),
                        .SizeInBytes = static_cast<UINT>(buffer.sizeInBytes),
                    },
            };

            buffer.cbvIndex = createCbv(cbvCreationDesc);
        }

        return buffer;
    }
} // namespace helios::gfx