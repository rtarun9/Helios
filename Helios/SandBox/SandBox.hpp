#pragma once

#include "Pch.hpp"

#include "Core/Engine.hpp"

namespace helios
{
	class SandBox : public Engine
	{
	public:
		SandBox(Config& config);

		virtual void OnInit() override;
		virtual void OnUpdate() override;
		virtual void OnRender() override;
		virtual void OnDestroy() override;

		void OnKeyDown(uint8_t keycode);
		void OnKeyUp(uint8_t keycode);

	private:
		void InitRendererCore();

		void EnableDebugLayer();
		void SelectAdapter();
		void CreateDevice();

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT);
		
		void CheckTearingSupport();
		void CreateSwapChain();

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t descriptorCount);

		void CreateBackBufferRenderTargetViews();

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT);
		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12CommandAllocator* commandAllocator, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT);

		void CreateFence();
		void CreateEventHandle();

		[[nodiscard]]
		uint64_t Signal(ID3D12CommandQueue* commandQueue, uint64_t& fenceValue);
		
		void WaitForFenceValue(uint64_t fenceValue, std::chrono::milliseconds duration = std::chrono::milliseconds::max());

		void Flush(ID3D12CommandQueue* commandQueue, uint64_t& fenceValue);

	private:
		// Number of SwapChain backbuffers.
		static constexpr uint8_t NUMBER_OF_FRAMES = 3;

		// DirectX12 objects
		Microsoft::WRL::ComPtr<ID3D12Device5> m_Device;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, NUMBER_OF_FRAMES> m_BackBuffers{};
		std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, NUMBER_OF_FRAMES>  m_CommandAllocator{};
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
		uint32_t m_RTVDescriptorSize{};
		uint32_t m_CurrentBackBufferIndex{};

		// Synchronization objects.
		Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
		uint64_t m_FenceValue{};
		std::array<uint64_t, NUMBER_OF_FRAMES> m_FrameFenceValues{};
		HANDLE m_FenceEvent{};

		bool m_VSync{true};
		bool m_IsTearingSupported{ false };
	};
}