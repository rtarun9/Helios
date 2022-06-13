#pragma once

#include "Helios.hpp"

// File with all constant buffer structs shared between C++ and HLSL.
#include "ConstantBuffers.hlsli"

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
	void PopulateCommandList(ID3D12GraphicsCommandList* commandList, ID3D12Resource* currentBackBuffer);

	void InitRendererCore();

	void LoadContent();

	void LoadMaterials();
	void LoadTextures(ID3D12GraphicsCommandList* commandList);
	void LoadModels(ID3D12GraphicsCommandList* commandList);
	void LoadLights(ID3D12GraphicsCommandList* commandList);
	void LoadRenderTargets(ID3D12GraphicsCommandList* commandList);
	void LoadCubeMaps();

	void RenderGBufferPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& dsvHandle);

	// Function will set the RTV and DSV to the rtv handle / dsv handle passed into the func.
	void RenderDeferredPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, bool enableIBL, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle & dsvHandle);
	
	// Resets the RTV and DSV to the handles passed into func.
	void RenderShadowPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle);
	void RenderShadowCSMPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle);
	
	void RenderLightSources(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material);
	void RenderGameObjects(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, bool enableIBL);
	
	void RenderSkyBox(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material);
	
	void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle);

	// CSM helper function.
	std::array<DirectX::XMVECTOR, 8> GetFrustumCorners(DirectX::XMMATRIX& projectionMatrix);
	std::pair<DirectX::XMMATRIX, DirectX::XMMATRIX> GetLightSpaceMatrix(float nearPlane, float farPlane);
	void CalculateCSMMatrices();

private:
	// Dimension of various textures.
	static constexpr uint32_t ENV_TEXTURE_DIMENSION = 1024u;
	static constexpr uint32_t CONVOLUTED_TEXTURE_DIMENSION = 128u;
	static constexpr uint32_t PREFILTER_TEXTURE_DIMENSION = 512u;
	static constexpr uint32_t BRDF_CONVOLUTION_TEXTURE_DIMENSION = 256u;

	static constexpr uint32_t SHADOW_DEPTH_MAP_DIMENSION = 1024u;

	static constexpr uint32_t DEFERRED_PASS_RENDER_TARGETS = 5u;

	static constexpr uint32_t CSM_DEPTH_MAPS = 5u;

	static constexpr float NEAR_PLANE = 0.5f;
	static constexpr float FAR_PLANE = 1000.0f;
	
	static constexpr std::array<float, CSM_DEPTH_MAPS + 1> CSM_CASCADES{ NEAR_PLANE, FAR_PLANE / 50.0f, FAR_PLANE / 25.0f, FAR_PLANE / 10.0f, FAR_PLANE / 2.0f,  FAR_PLANE };

	
	D3D12_VIEWPORT m_Viewport{};
	D3D12_RECT m_ScissorRect{ .left = 0, .top = 0, .right = LONG_MAX, .bottom = LONG_MAX };

	helios::gfx::DepthStencilBuffer m_DepthBuffer{};
	
	helios::gfx::DepthStencilBuffer m_ShadowDepthBuffer{};
	
	// This is used here to prevent modifying the depth buffer value when deferred lighting pass occurs.
	// The optimal solution will be to copy the depth buffer value into the buffer.
	helios::gfx::DepthStencilBuffer m_DeferredDepthBuffer{};

	std::array<helios::gfx::DepthStencilBuffer, CSM_DEPTH_MAPS> m_CSMDepthMaps{};

	// Application Data.
	std::map<std::wstring_view, helios::gfx::Material> m_Materials{};
	std::map<std::wstring_view, helios::gfx::Texture> m_Textures{};
	std::map<std::wstring_view, helios::Model> m_PBRModels{};

	std::vector<helios::gfx::Light> m_Lights{};

	// Data for IBL / Skybox.
	helios::gfx::Texture m_EnvironmentTexture{};
	helios::gfx::Texture m_IrradianceMapTexture{};
	helios::gfx::Texture m_PreFilterMapTexture{};
	helios::gfx::Texture m_BRDFConvolutionTexture{};

	helios::Model m_SkyBoxModel{};

	helios::UIManager m_UIManager{};

	// Transform data.
	float m_FOV{ 45.0f };

	DirectX::XMMATRIX m_ViewMatrix{};
	DirectX::XMMATRIX m_ProjectionMatrix{};

	DirectX::XMMATRIX m_LightViewMatrix{};
	DirectX::XMMATRIX m_LightProjectionMatrix{};
	DirectX::XMMATRIX m_CSMLightViewMatrix{};
	DirectX::XMMATRIX m_CSMLightProjectionMatrix{};
	
	std::array<std::pair<DirectX::XMMATRIX, DirectX::XMMATRIX>, CSM_DEPTH_MAPS> m_CSMLightMatrices{};

	// Used till CSM is implemented.
	float m_BackOffDistance{10.0f};
	float m_Extents{ 30.0f };
	D3D12_VIEWPORT m_ShadowViewport{};

	// Render target Data.
	helios::gfx::RenderTarget m_OffscreenRT{};
	helios::gfx::RenderTarget m_GBuffer{};
	helios::gfx::RenderTarget m_DeferredRT{};

	helios::gfx::ConstantBuffer<ShadowMappingData> m_ShadowMappingData{};
	std::array<helios::gfx::ConstantBuffer<ShadowMappingData>, CSM_DEPTH_MAPS> m_CSMShadowMappingData{};

	float m_ZMultiplier{ 10.0f };

	helios::gfx::ConstantBuffer<RenderTargetSettings> m_RenderTargetSettingsData{};
};