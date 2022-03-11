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

	void CheckTearingSupport();
	void CreateSwapChain();

	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t descriptorCount);

	void CreateBackBufferRenderTargetViews();
	void CreateDepthBuffer();

private:
	// Number of SwapChain backbuffers.
	static constexpr uint8_t NUMBER_OF_FRAMES = 3;

	// Core DirectX12 objects and data.
	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;

	helios::gfx::CommandQueue m_CommandQueue{};

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, NUMBER_OF_FRAMES> m_BackBuffers{};

	Microsoft::WRL::ComPtr<ID3D12Debug1> m_DebugInterface;

	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ScissorRect{ .left = 0, .top = 0, .right = LONG_MAX, .bottom = LONG_MAX };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;

	// Descriptor heaps.
	helios::gfx::Descriptor m_RTVDescriptor{};
	helios::gfx::Descriptor m_DSVDescriptor{};
	helios::gfx::Descriptor m_SRV_CBV_Descriptor{};

	// Synchronization objects.
	std::array<uint64_t, NUMBER_OF_FRAMES> m_FrameFenceValues{};
	
	// SwapChain / presentation control objects.
	bool m_VSync{true};
	bool m_IsTearingSupported{ false };
	uint32_t m_CurrentBackBufferIndex{};
	bool m_IsFullScreen{ false };

	// Application Data.

	helios::Camera m_Camera{};

	// Transform data.
	float m_FOV{ 45.0f };
	
	// Cube data.
	DirectX::XMMATRIX m_ModelMatrix{};
	DirectX::XMMATRIX m_ViewMatrix{};
	DirectX::XMMATRIX m_ProjectionMatrix{};

	helios::Transform m_IcoSphereTransform{};
	helios::Transform m_FloorTransform{};

	helios::Model m_IcoSphere{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

	helios::gfx::Texture m_Texture{};
	helios::gfx::Texture m_FloorTexture{};

	// Light source data.
	DirectX::XMMATRIX m_LightModelMatrix{DirectX::XMMatrixIdentity()};
	DirectX::XMFLOAT4 m_LightPosition{ DirectX::XMFLOAT4(1.5f, 0.5f, 0.f, 1.0f) };

	helios::Model m_LightSource{};
	helios::Model m_Floor{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_LightRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_LightPSO;
};