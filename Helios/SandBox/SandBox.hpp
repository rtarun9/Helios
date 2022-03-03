#pragma once

#include "Helios.hpp"

class SandBox : public helios::Engine
{
public:
	SandBox(helios::Config& config);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

	void OnKeyAction(uint8_t keycode, bool isKeyDown) override;
	void OnResize() override;

private:
	void InitRendererCore();

	void LoadContent();

	void EnableDebugLayer();
	void SelectAdapter();
	void CreateDevice();

	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT);

	void CheckTearingSupport();
	void CreateSwapChain();

	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t descriptorCount);

	void CreateBackBufferRenderTargetViews();
	void CreateDepthBuffer();

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

	// Core DirectX12 objects and data.
	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, NUMBER_OF_FRAMES> m_BackBuffers{};
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, NUMBER_OF_FRAMES>  m_CommandAllocator{};
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

	Microsoft::WRL::ComPtr<ID3D12Debug1> m_DebugInterface;

	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ScissorRect{ .left = 0, .top = 0, .right = LONG_MAX, .bottom = LONG_MAX };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;

	// Descriptor heaps.
	helios::gfx::Descriptor m_RTVDescriptor{};
	helios::gfx::Descriptor m_DSVDescriptor{};
	helios::gfx::Descriptor m_SRV_CBV_Descriptor{};

	// Root signature & PSO.
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

	// Synchronization objects.
	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
	uint64_t m_FenceValue{};
	std::array<uint64_t, NUMBER_OF_FRAMES> m_FrameFenceValues{};
	HANDLE m_FenceEvent{};
	
	// SwapChain / presentation control objects.
	bool m_VSync{true};
	bool m_IsTearingSupported{ false };
	uint32_t m_CurrentBackBufferIndex{};
	bool m_IsFullScreen{ false };

	// Application Data.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;

	helios::Camera m_Camera{};

	// Transform data.
	float m_FOV{ 45.0f };
	DirectX::XMMATRIX m_ModelMatrix{};
	DirectX::XMMATRIX m_ViewMatrix{};
	DirectX::XMMATRIX m_ProjectionMatrix{};

	helios::Model m_Cube{};
};