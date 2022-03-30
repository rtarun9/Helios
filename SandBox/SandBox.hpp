#pragma once

#include "Helios.hpp"

class SandBox : public helios::Engine
{
	struct alignas(256) MaterialData
	{
		DirectX::XMFLOAT3 albedo{};
		float metallicFactor{};
		float roughnessFactor{};
		float ao{};
	};

public:
	SandBox(helios::Config& config);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

	void OnKeyAction(uint8_t keycode, bool isKeyDown) override;
	void OnResize() override;

private:
	void PopulateCommandList(ID3D12GraphicsCommandList* commandList, ID3D12Resource* currentBackBuffer);

	void InitRendererCore();

	void LoadContent();
	
	void LoadMaterials();
	void LoadTextures(ID3D12GraphicsCommandList* commandList);
	void LoadModels(ID3D12GraphicsCommandList* commandList);

	void EnableDebugLayer();
	void SelectAdapter();
	void CreateDevice();

	void CheckTearingSupport();
	void CreateSwapChain();

	void CreateBackBufferRenderTargetViews();
	void CreateDepthBuffer();

private:
	// Number of SwapChain backbuffers.
	static constexpr uint8_t NUMBER_OF_FRAMES = 3;

	// Core DirectX12 objects and data.
	Microsoft::WRL::ComPtr<ID3D12Device8> m_Device;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;

	helios::gfx::CommandQueue m_CommandQueue{};

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, NUMBER_OF_FRAMES> m_BackBuffers{};

	Microsoft::WRL::ComPtr<ID3D12Debug3> m_DebugInterface;

	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ScissorRect{ .left = 0, .top = 0, .right = LONG_MAX, .bottom = LONG_MAX };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;

	// Descriptor heaps.
	helios::gfx::Descriptor m_RTVDescriptor{};
	helios::gfx::Descriptor m_DSVDescriptor{};
	helios::gfx::Descriptor m_SRV_CBV_UAV_Descriptor{};

	// Synchronization objects.
	std::array<uint64_t, NUMBER_OF_FRAMES> m_FrameFenceValues{};

	// SwapChain / presentation control objects.
	bool m_VSync{ true };
	bool m_IsTearingSupported{ false };
	uint32_t m_CurrentBackBufferIndex{};
	bool m_IsFullScreen{ false };

	// Application Data.
	std::map<std::wstring_view, helios::gfx::Material> m_Materials{};
	std::map<std::wstring_view, helios::gfx::Texture> m_Textures{};
	std::map<std::wstring_view, helios::Model> m_GameObjects{};

	helios::Model m_Sphere{};
	helios::gfx::ConstantBuffer<MaterialData> m_PBRMaterial{};

	helios::Model m_LightSource{};

	helios::Camera m_Camera{};

	helios::UIManager m_UIManager{};

	// Transform data.
	float m_FOV{ 45.0f };

	DirectX::XMMATRIX m_ViewMatrix{};
	DirectX::XMMATRIX m_ProjectionMatrix{};

	// Render target Data.
	helios::gfx::RenderTarget m_OffscreenRT{};
};