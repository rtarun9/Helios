#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"
#include "CommandQueue.hpp"
#include "GraphicsContext.hpp"
#include "MemoryAllocator.hpp"
#include "PipelineState.hpp"

namespace helios::gfx
{
	// Abstraction for creating / destroying various graphics resources.
	// Encapsulates most renderer resources / objects in use : the swapchain, descriptor heaps, command queue's, etc.
	// Inspired from : https://alextardif.com/DX12Tutorial.html.
	class Device
	{
	public:
		Device();
		~Device();

		// Getters
		ID3D12Device5* GetDevice() const { return mDevice.Get(); }
		IDXGIFactory6* GetFactory() const { return mFactory.Get(); }
		IDXGIAdapter4* GetAdapter() const { return mAdapter.Get(); }
		IDXGISwapChain4* GetSwapChain() const { return mSwapChain.Get(); }

		Descriptor* GetRtvDescriptor() const { return mRtvDescriptor.get(); }
		Descriptor* GetSrvCbvUavDescriptor() const { return mSrvCbvUavDescriptor.get(); }
		Descriptor* GetDsvDescriptor() const { return mDsvDescriptor.get(); }

		CommandQueue* GetGraphicsCommandQueue() const { return mGraphicsCommandQueue.get(); }
		CommandQueue* GetComputeCommandQueue() const { return mComputeCommandQueue.get(); }

		MemoryAllocator* GetMemoryAllocator() const { return mMemoryAllocator.get(); }
		BackBuffer* GetCurrentBackBuffer()  { return &mBackBuffers[mCurrentBackBufferIndex]; }

		std::unique_ptr<GraphicsContext> GetGraphicsContext() { return std::move(std::make_unique<GraphicsContext>(*this)); }
		
		// Device resources are the device, adapters, queues, descriptor heaps, etc.
		// Swapchain resources are kept seperate for resources with depended upon the window.
		void InitDeviceResources();
		void InitSwapChainResources();

		void CreateBackBufferRTVs();
		void ResizeBuffers();

		// Rendering related functions.
		void BeginFrame();
		void EndFrame();

		void ExecuteContext(std::unique_ptr<GraphicsContext> graphicsContext);
		void Present();

		// Helper creation functions.
		uint32_t CreateSrv(const SrvCreationDesc& srvCreationDesc, ID3D12Resource* resource) const;
		uint32_t CreateRtv(const RtvCreationDesc& rtvCreationDesc, ID3D12Resource* resource) const;
		uint32_t CreateDsv(const DsvCreationDesc& dsvCreationDesc, ID3D12Resource* resource) const;
		uint32_t CreateCbv(const CbvCreationDesc& cbvCreationDesc) const;

		Buffer CreateBuffer(const BufferCreationDesc& bufferCreationDesc, const void *data = nullptr) const;
		Texture CreateTexture(const TextureCreationDesc& textureCreationDesc);
		PipelineState CreatePipelineState(const GraphicsPipelineStateCreationDesc& graphicsPipelineStateCreationDesc) const;

	private:
		// Number of SwapChain backbuffers.
		static constexpr uint8_t NUMBER_OF_FRAMES = 3u;

		Microsoft::WRL::ComPtr<ID3D12Device5> mDevice{};
		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> mDebugDevice{};
		Microsoft::WRL::ComPtr<ID3D12Debug3> mDebugInterface{};

		Microsoft::WRL::ComPtr<IDXGIFactory6> mFactory{};
		Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter{};
		Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain{};
		
		bool mVSync{true};
		bool mTearingSupported{};

		uint32_t mCurrentBackBufferIndex{};
		std::array<BackBuffer, NUMBER_OF_FRAMES> mBackBuffers{};

		std::array<uint64_t, NUMBER_OF_FRAMES> mFrameFenceValues{};

		std::unique_ptr<MemoryAllocator> mMemoryAllocator{};

		std::unique_ptr<Descriptor> mRtvDescriptor{};
		std::unique_ptr<Descriptor> mDsvDescriptor{};
		std::unique_ptr<Descriptor> mSrvCbvUavDescriptor{};

		std::unique_ptr<CommandQueue> mGraphicsCommandQueue{};
		std::unique_ptr<CommandQueue> mComputeCommandQueue{};
		std::unique_ptr<CommandQueue> mCopyCommandQueue{};
	};
}