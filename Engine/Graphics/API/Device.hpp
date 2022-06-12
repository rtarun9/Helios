#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	// Abstraction for creating / destroying various graphics resources.
	// Encapsulates most renderer resources / objects in use : the swapchain, descriptor heaps, command queue's, etc.
	// Inspired from : https://alextardif.com/DX12Tutorial.html.
	class Device
	{
	public:
		Device();
		~Device() = default;

		ID3D12Device5* GetDevice() const { return mDevice.Get(); }
		IDXGIFactory7* GetFactory() const { return mFactory.Get(); }
		IDXGIAdapter4* GetAdapter() const { return mAdapter.Get(); }
		IDXGISwapChain4* GetSwapChain() const { return mSwapChain.Get(); }

		// Device resources are the device, adapters, queues, descriptor heaps, etc.
		// Swapchain resources are kept seperate for resources with depended upon the window.
		void InitDeviceResources();
		void InitSwapChainResources();


	private:
		// Number of SwapChain backbuffers.
		static constexpr uint8_t NUMBER_OF_FRAMES = 3u;

		Microsoft::WRL::ComPtr<ID3D12Device5> mDevice;
		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> mDebugDevice;
		Microsoft::WRL::ComPtr<ID3D12Debug3> mDebugInterface;

		Microsoft::WRL::ComPtr<IDXGIFactory7> mFactory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;
		bool mVSync{};
	};
}