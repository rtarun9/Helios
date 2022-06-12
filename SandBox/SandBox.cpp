#include "Pch.hpp"
#include "SandBox.hpp"

#include "BindlessRS.hlsli" 

using namespace helios;
using namespace DirectX;

SandBox::SandBox(Config& config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	InitRendererCore();

	LoadContent();
}

void SandBox::OnUpdate()
{
	m_Camera.Update(static_cast<float>(Application::GetTimer().GetDeltaTime()));

	m_ViewMatrix = dx::XMMatrixTranspose(m_Camera.GetViewMatrix());
	m_ProjectionMatrix = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XMConvertToRadians(m_FOV), m_AspectRatio, 0.1f, 1000.0f));

	// Assuming that the directional light is at index 0 (i.e the light index is 0u).
	auto directionalLightPosition = gfx::Light::GetLightData().lightPosition[0u];
	auto directionalLightPositionVector = dx::XMLoadFloat4(&directionalLightPosition);

	dx::XMVECTOR lightPosition = DirectX::XMVectorZero() - m_BackOffDistance * dx::XMVector4Normalize(directionalLightPositionVector);

	m_LightViewMatrix = dx::XMMatrixTranspose(dx::XMMatrixLookAtLH(lightPosition, DirectX::XMVectorZero(), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	m_LightProjectionMatrix = dx::XMMatrixTranspose(dx::XMMatrixOrthographicOffCenterLH(-m_Extents, m_Extents, -m_Extents, m_Extents, 0.1f, 100.0f));

	m_ShadowMappingData.GetBufferData() = { .lightViewMatrix = m_LightViewMatrix, .lightProjectionMatrix = m_LightProjectionMatrix};

	CalculateCSMMatrices();

	uint32_t csmIndex{};
	for (auto& csmShadowMappingData : m_CSMShadowMappingData)
	{
		csmShadowMappingData.GetBufferData() = { .lightViewMatrix = m_CSMLightMatrices[csmIndex].first, .lightProjectionMatrix = m_CSMLightMatrices[csmIndex].second };
		csmShadowMappingData.Update();
		csmIndex++;
	}

	m_ShadowMappingData.Update();
	m_RenderTargetSettingsData.Update();

}

void SandBox::OnRender()
{
	auto commandList = m_CommandQueue.GetCommandList();
	wrl::ComPtr<ID3D12Resource> currentBackBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

	PopulateCommandList(commandList.Get(), currentBackBuffer.Get());

	m_FrameFenceValues[m_CurrentBackBufferIndex] = m_CommandQueue.ExecuteCommandList(commandList.Get());

	uint32_t syncInterval = m_VSync ? 1u : 0u;
	uint32_t presentFlags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0u;

	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	m_CommandQueue.WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
	
	m_FrameIndex++;
}

void SandBox::OnDestroy()
{
	m_CommandQueue.FlushQueue();

	m_UIManager.ShutDown();
}

void SandBox::OnKeyAction(uint8_t keycode, bool isKeyDown) 
{
	m_Camera.HandleInput(keycode, isKeyDown);
}

void SandBox::OnResize() 
{
	if (m_Width != Application::GetClientWidth() || m_Height != Application::GetClientHeight())
	{
		m_CommandQueue.FlushQueue();

		for (int i = 0; i < NUMBER_OF_FRAMES; i++)
		{
			m_BackBuffers[i].Reset();
			m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
		}
		
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(NUMBER_OF_FRAMES, Application::GetClientWidth(), Application::GetClientHeight(), swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		m_Width = Application::GetClientWidth();
		m_Height = Application::GetClientHeight();

		CreateBackBufferRenderTargetViews();

		m_DepthBuffer.Init(m_Device.Get(), m_DSVDescriptor, m_SRV_CBV_UAV_Descriptor, DXGI_FORMAT_D24_UNORM_S8_UINT, m_Width, m_Height, L"Depth Buffer");
	}
}

void SandBox::PopulateCommandList(ID3D12GraphicsCommandList* commandList, ID3D12Resource* currentBackBuffer)
{
	m_UIManager.FrameStart();
	
	m_UIManager.Begin(L"Scene Control");
	
	if (ImGui::TreeNode("Render Target Settings"))
	{
		ImGui::SliderFloat("Exposure", &m_RenderTargetSettingsData.GetBufferData().exposure, 0.1f, 5.0f);
		ImGui::TreePop();
	}

	ImGui::SliderFloat("Camera Speed", &m_Camera.m_MovementSpeed, 1.0f, 1250.0f);

	ImGui::SliderFloat("Back off distance", &m_BackOffDistance,-5.0f, 250.0f);
	ImGui::SliderFloat("Extents", &m_Extents, 1.0f, 100.0f);

	// Test code.
	static bool enableIBL{ false };
	ImGui::Checkbox("Enable IBL", &enableIBL);

	ImGui::SliderFloat("Z multiplier", &m_ZMultiplier, 0.1f, 25.0f);

	for (auto& [modelName, model] : m_PBRModels)
	{
		model.UpdateTransformData(commandList, m_ProjectionMatrix, m_ViewMatrix);
	}

	auto shadowDepthBufferSRV = m_SRV_CBV_UAV_Descriptor.GetDescriptorHandleFromIndex(m_ShadowDepthBuffer.GetSRVIndex());
	m_UIManager.Image(shadowDepthBufferSRV);

	for (auto& csmShadowDepthBuffer : m_CSMDepthMaps)
	{
		auto shadowDepthBufferSRV = m_SRV_CBV_UAV_Descriptor.GetDescriptorHandleFromIndex(csmShadowDepthBuffer.GetSRVIndex());
		m_UIManager.Image(shadowDepthBufferSRV);
	}

 	for (auto& light : m_Lights)
	{
		light.UpdateTransformData(commandList, m_ProjectionMatrix, m_ViewMatrix);
	}


	m_SkyBoxModel.UpdateTransformData(commandList, m_ProjectionMatrix, m_ViewMatrix);
	


	static std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	m_UIManager.SetClearColor(clearColor);


	m_UIManager.End();

	// Set the necessary states

	commandList->RSSetViewports(1u, &m_Viewport);
	commandList->RSSetScissorRects(1u, &m_ScissorRect);

	// Inidicate render target will be used as RTV.
	gfx::utils::TransitionResource(commandList, m_OffscreenRT.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Record rendering commands
	auto rtvHandle = m_RTVDescriptor.GetDescriptorHandleFromIndex(m_OffscreenRT.GetRTVIndex());
	auto dsvHandle = m_DSVDescriptor.GetDescriptorHandleFromIndex(m_DepthBuffer.GetBufferIndex());
	
	gfx::utils::ClearRTV(commandList, rtvHandle, clearColor);
	gfx::utils::ClearDepthBuffer(commandList, dsvHandle);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);

	// Rendering commands

	// Set descriptor heaps and root signature once per command list.
	gfx::utils::SetDescriptorHeaps(commandList, m_SRV_CBV_UAV_Descriptor);
	gfx::Material::BindRootSignature(commandList);

	RenderShadowPass(commandList, m_Materials[L"ShadowPassMaterial"], rtvHandle, dsvHandle);
	RenderShadowCSMPass(commandList, m_Materials[L"ShadowPassMaterial"], rtvHandle, dsvHandle);
	RenderGBufferPass(commandList, m_Materials[L"GPassMaterial"], dsvHandle);
	RenderDeferredPass(commandList, m_Materials[L"PBRDeferredMaterial"], enableIBL, rtvHandle,  dsvHandle);
	RenderLightSources(commandList, m_Materials[L"LightMaterial"]);
	RenderSkyBox(commandList, m_Materials[L"SkyBoxMaterial"]);

	gfx::utils::TransitionResource(commandList, m_OffscreenRT.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Render to back buffer.
	gfx::utils::TransitionResource(commandList, currentBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	rtvHandle = m_RTVDescriptor.GetDescriptorHandleForStart();
	m_RTVDescriptor.Offset(rtvHandle, m_CurrentBackBufferIndex);
	
	RenderToBackBuffer(commandList, m_Materials[L"OffscreenRTMaterial"], rtvHandle, dsvHandle);

	m_UIManager.FrameEnd(commandList);

	gfx::utils::TransitionResource(commandList, currentBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void SandBox::InitRendererCore()
{
	CreateFactory();
	EnableDebugLayer();
	SelectAdapter();

	mDevice = std::make_unique<gfx::Device>(m_Adapter.Get());
	m_Device = mDevice->GetDevice();

	m_CommandQueue.Init(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Main Command Queue");
	m_ComputeCommandQueue.Init(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Compute Command Queue");

	CheckTearingSupport();
	CreateSwapChain();

	m_RTVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"RTV Descriptor");

	m_DSVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"DSV Descriptor");

	m_SRV_CBV_UAV_Descriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1020u, L"SRV_CBV_UAV Descriptor");

	CreateBackBufferRenderTargetViews();

	m_DepthBuffer.Init(m_Device.Get(), m_DSVDescriptor, m_SRV_CBV_UAV_Descriptor, DXGI_FORMAT_R32_TYPELESS, m_Width, m_Height, L"Depth Stencil Buffer");

	m_Viewport =
	{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = static_cast<float>(m_Width),
		.Height = static_cast<float>(m_Height),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f
	};
	
	m_UIManager.Init(m_Device.Get(), NUMBER_OF_FRAMES, m_SRV_CBV_UAV_Descriptor);
}

void SandBox::LoadContent()
{
	gfx::Material::CreateBindlessRootSignature(m_Device.Get(), L"Shaders/BindlessRS.cso");
	
	auto commandList = m_CommandQueue.GetCommandList();
	
	m_Camera.Init(m_Device.Get(), commandList.Get(), m_SRV_CBV_UAV_Descriptor);

	LoadMaterials();
	LoadTextures(commandList.Get());

	m_CommandQueue.ExecuteAndFlush(commandList.Get());

	// Cube maps require data that gets ready in LoadTextures and LoadMaterials, hence why they are executed before the cube map function.
	// This behavious is not ideal, and will be fixed soon.
	std::thread cubeMapThread(std::bind(&SandBox::LoadCubeMaps, this));
	
	commandList = m_CommandQueue.GetCommandList();

	LoadModels(commandList.Get());
	LoadLights(commandList.Get());
	LoadRenderTargets(commandList.Get());

	// Close command list and execute it (for the initial setup).
	m_FrameFenceValues[m_CurrentBackBufferIndex] =  m_CommandQueue.ExecuteCommandList(commandList.Get());
	m_CommandQueue.FlushQueue();
	
	cubeMapThread.join();
}

void SandBox::LoadMaterials()
{
	m_Materials[L"LightMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{.vsShaderPath = L"Shaders/LightVS.cso", .psShaderPath = L"Shaders/LightPS.cso", .rtvCount = 1u, .format = DXGI_FORMAT_R16G16B16A16_FLOAT, .depthFormat = DXGI_FORMAT_D32_FLOAT}, L"Light Material");
	m_Materials[L"PBRMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{ .vsShaderPath = L"Shaders/PBRVS.cso", .psShaderPath = L"Shaders/PBRPS.cso", .rtvCount = 1u, .format = DXGI_FORMAT_R16G16B16A16_FLOAT,  .depthFormat = DXGI_FORMAT_D32_FLOAT }, L"PBR Material");
	m_Materials[L"OffscreenRTMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{ .vsShaderPath = L"Shaders/OffscreenRTVS.cso", .psShaderPath = L"Shaders/OffscreenRTPS.cso", .rtvCount = 1u, .format = DXGI_FORMAT_R8G8B8A8_UNORM ,.depthFormat = DXGI_FORMAT_D32_FLOAT }, L"Offscreen RT Material");

	m_Materials[L"GPassMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{ .vsShaderPath = L"Shaders/GPassVS.cso", .psShaderPath = L"Shaders/GPassPS.cso", .rtvCount = DEFERRED_PASS_RENDER_TARGETS, .format = DXGI_FORMAT_R16G16B16A16_FLOAT, .depthFormat = DXGI_FORMAT_D32_FLOAT }, L"G Pass Material");
	m_Materials[L"PBRDeferredMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{ .vsShaderPath = L"Shaders/PBRDeferredVS.cso", .psShaderPath = L"Shaders/PBRDeferredPS.cso", .rtvCount = 1u, .format = DXGI_FORMAT_R16G16B16A16_FLOAT, .depthFormat = DXGI_FORMAT_D32_FLOAT }, L"PBR Deferred Material");

	m_Materials[L"ShadowPassMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{ .vsShaderPath = L"Shaders/ShadowVS.cso", .psShaderPath = L"Shaders/ShadowPS.cso", .rtvCount = 1u, .format = DXGI_FORMAT_D32_FLOAT, .depthFormat = DXGI_FORMAT_D32_FLOAT }, L"Shadow Pass Material");

	m_Materials[L"SkyBoxMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::GraphicsMaterialData{ .vsShaderPath = L"Shaders/SkyBoxVS.cso", .psShaderPath = L"Shaders/SkyBoxPS.cso", .rtvCount = 1u, .format = DXGI_FORMAT_R16G16B16A16_FLOAT, .depthFormat = DXGI_FORMAT_D32_FLOAT, .depthComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL }, L"Sky Box Material");
	
	m_Materials[L"EquirectEnvironmentMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::ComputeMaterialData{ .csShaderPath = L"Shaders/CubeFromEquirectTextureCS.cso" }, L"Cube From Equirect Material");
	m_Materials[L"CubeMapConvolutionMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::ComputeMaterialData{ .csShaderPath = L"Shaders/CubeMapConvolutionCS.cso" }, L"Cube Map Convolution Material");
	m_Materials[L"PreFilterCubeMapMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::ComputeMaterialData{ .csShaderPath = L"Shaders/PreFilterCubeMapCS.cso" }, L"Pre Filter Cube Map");

	m_Materials[L"BRDFConvolutionMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), gfx::ComputeMaterialData{ .csShaderPath = L"Shaders/BRDFConvolutionCS.cso" }, L"BRDF Convolution Material");
}

void SandBox::LoadTextures(ID3D12GraphicsCommandList* commandList)
{
	m_Textures[L"EquirectEnvironmentTexture"].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::HDRTextureData{ L"Assets/Textures/Environment.hdr", 1u, DXGI_FORMAT_R32G32B32A32_FLOAT }, L"Environment Equirect Texture");

	m_EnvironmentTexture.Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::UAVTextureData{ ENV_TEXTURE_DIMENSION, ENV_TEXTURE_DIMENSION, 6u, 6u, DXGI_FORMAT_R16G16B16A16_FLOAT }, L"Environment Cube Box Texture UAV");
	m_IrradianceMapTexture.Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::UAVTextureData{ CONVOLUTED_TEXTURE_DIMENSION, CONVOLUTED_TEXTURE_DIMENSION, 6u, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT }, L"Irradiance Convoluted Cube Map");
	
	m_PreFilterMapTexture.Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::UAVTextureData{ PREFILTER_TEXTURE_DIMENSION, PREFILTER_TEXTURE_DIMENSION, 6u, 7u, DXGI_FORMAT_R16G16B16A16_FLOAT }, L"Prefilter Specular Texture Map");

	m_BRDFConvolutionTexture.Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::UAVTextureData{BRDF_CONVOLUTION_TEXTURE_DIMENSION, BRDF_CONVOLUTION_TEXTURE_DIMENSION, 1u, 1u, DXGI_FORMAT_R16G16_FLOAT}, L"BRDF Convolution Texture");
}

void SandBox::LoadModels(ID3D12GraphicsCommandList* commandList)
{
	m_SkyBoxModel.Init(m_Device.Get(), commandList, L"Assets/Models/Cube/glTF/Cube.gltf", m_SRV_CBV_UAV_Descriptor, L"Sky Box Model");

#if 1
	m_PBRModels[L"Spheres"].Init(m_Device.Get(), commandList, L"Assets/Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", m_SRV_CBV_UAV_Descriptor, L"Spheres");
	m_PBRModels[L"Spheres"].GetTransform().translate = { 10.0f, 0.0f, 0.0f };

	m_PBRModels[L"SciFi Helmet"].Init(m_Device.Get(), commandList, L"Assets/Models/SciFiHelmet/glTF/SciFiHelmet.gltf", m_SRV_CBV_UAV_Descriptor, L"SciFi Helmet");

	m_PBRModels[L"Damaged Helmet"].Init(m_Device.Get(), commandList, L"Assets/Models/DamagedHelmet/glTF/DamagedHelmet.gltf", m_SRV_CBV_UAV_Descriptor, L"Damaged Helmet");

#endif

#if 1
	m_PBRModels[L"Sponza"].Init(m_Device.Get(), commandList, L"Assets/Models/Sponza/glTF/Sponza.gltf", m_SRV_CBV_UAV_Descriptor, L"Sponza");
	m_PBRModels[L"Sponza"].GetTransform().scale = { 0.1f, 0.1f, 0.1f };

#endif
}

void SandBox::LoadRenderTargets(ID3D12GraphicsCommandList* commandList)
{
	// Create render targets and thier Root signature and PSO.
	gfx::RenderTarget::InitBuffers(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor);

	m_OffscreenRT.Init(m_Device.Get(), commandList, DXGI_FORMAT_R16G16B16A16_FLOAT, m_RTVDescriptor, m_SRV_CBV_UAV_Descriptor, m_Width, m_Height, 1u, L"Offscreen RT");
	m_DeferredRT.Init(m_Device.Get(), commandList, DXGI_FORMAT_R16G16B16A16_FLOAT, m_RTVDescriptor, m_SRV_CBV_UAV_Descriptor, m_Width, m_Height, 1u, L"Deferred RT");
	m_GBuffer.Init(m_Device.Get(), commandList, DXGI_FORMAT_R16G16B16A16_FLOAT, m_RTVDescriptor, m_SRV_CBV_UAV_Descriptor, m_Width, m_Height, DEFERRED_PASS_RENDER_TARGETS, L"G Buffer");

	m_DeferredDepthBuffer.Init(m_Device.Get(), m_DSVDescriptor, m_SRV_CBV_UAV_Descriptor, DXGI_FORMAT_R32_TYPELESS, m_Width, m_Height, L"Deferred Depth Buffer");
	m_ShadowDepthBuffer.Init(m_Device.Get(), m_DSVDescriptor, m_SRV_CBV_UAV_Descriptor, DXGI_FORMAT_R32_TYPELESS, SHADOW_DEPTH_MAP_DIMENSION, SHADOW_DEPTH_MAP_DIMENSION, L"Shadow Depth Buffer");
	gfx::utils::TransitionResource(commandList, m_ShadowDepthBuffer.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_ShadowMappingData.Init(m_Device.Get(), commandList, ShadowMappingData{}, m_SRV_CBV_UAV_Descriptor, L"Shadow Mapping Data CBuffer");

	for (uint32_t i = 0; i < CSM_DEPTH_MAPS; ++i)
	{
		m_CSMDepthMaps[i].Init(m_Device.Get(), m_DSVDescriptor, m_SRV_CBV_UAV_Descriptor, DXGI_FORMAT_R32_TYPELESS, SHADOW_DEPTH_MAP_DIMENSION, SHADOW_DEPTH_MAP_DIMENSION, L"CSM SHadow Depth Buffer" + std::to_wstring(i));
		gfx::utils::TransitionResource(commandList, m_CSMDepthMaps[i].GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_CSMShadowMappingData[i].Init(m_Device.Get(), commandList, ShadowMappingData{}, m_SRV_CBV_UAV_Descriptor, L"CSM Shadow Mapping Data CBuffer " + std::to_wstring(i));
	}

	m_ShadowViewport =
	{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = static_cast<float>(SHADOW_DEPTH_MAP_DIMENSION),
		.Height = static_cast<float>(SHADOW_DEPTH_MAP_DIMENSION),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f
	};

	m_RenderTargetSettingsData.Init(m_Device.Get(), commandList, RenderTargetSettings{ .exposure = 1.0f }, m_SRV_CBV_UAV_Descriptor, L"Render Target Settings Data");
}

void SandBox::LoadLights(ID3D12GraphicsCommandList* commandList)
{
	m_Lights.resize(TOTAL_LIGHTS);
	gfx::Light::InitLightDataCBuffer(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor);

	uint32_t lightIndex{};

	// Load directional lights.
	m_Lights[lightIndex].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::DirectionalLightData{ .sunAngle = -153.0f, .lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, lightIndex++);

	// Load point lights and place them in a grid order.
	for (; lightIndex < TOTAL_POINT_LIGHTS + 1; ++lightIndex)
	{
		m_Lights[lightIndex].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, gfx::PointLightData{ .radius =  0.02f, .lightPosition = DirectX::XMFLOAT4(12.0f * (lightIndex % 10) - 22.0f, 2.0f,  6.0f * (lightIndex / 10) - 18.0f, 1.0f), .lightColor = DirectX::XMFLOAT4(1.0f - (float)lightIndex / TOTAL_POINT_LIGHTS, 1.0f,  (float)lightIndex / TOTAL_POINT_LIGHTS, 1.0f)}, lightIndex);
	}
}

void SandBox::LoadCubeMaps()
{
	// Run compute shader to generate cube map from equirectangular texture.
	{
		auto commandList = m_ComputeCommandQueue.GetCommandList();

		gfx::utils::TransitionResource(commandList.Get(), m_EnvironmentTexture.GetTextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		gfx::utils::SetDescriptorHeaps(commandList.Get(), m_SRV_CBV_UAV_Descriptor);
		gfx::Material::BindRootSignatureCS(commandList.Get());

		m_Materials[L"EquirectEnvironmentMaterial"].BindPSO(commandList.Get());

		uint32_t size{ ENV_TEXTURE_DIMENSION };
		for (uint32_t i = 0; i < m_EnvironmentTexture.GetMipLevels(); ++i)
		{
			m_EnvironmentTexture.CreateUAV(m_Device.Get(), m_SRV_CBV_UAV_Descriptor, i);

			CubeFromEquirectRenderResources cubeFromEquirectRenderResources
			{
				.textureIndex = m_Textures[L"EquirectEnvironmentTexture"].GetTextureIndex(),
				.outputTextureIndex = m_EnvironmentTexture.GetUAVIndex(i)
			};

			commandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &cubeFromEquirectRenderResources, 0u);

			const uint32_t numGroups = std::max(1u, size / 32u);

			commandList->Dispatch(numGroups, numGroups, 6u);

			size /= 2;
		}
		
		gfx::utils::TransitionResource(commandList.Get(), m_EnvironmentTexture.GetTextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

		m_ComputeCommandQueue.ExecuteAndFlush(commandList.Get());
	}

	// Run compute shader to generate irradiance map from above generated cube map texture.
	{
		auto commandList = m_ComputeCommandQueue.GetCommandList();

		gfx::utils::SetDescriptorHeaps(commandList.Get(), m_SRV_CBV_UAV_Descriptor);
		gfx::Material::BindRootSignatureCS(commandList.Get());

		gfx::utils::TransitionResource(commandList.Get(), m_IrradianceMapTexture.GetTextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		m_Materials[L"CubeMapConvolutionMaterial"].BindPSO(commandList.Get());

		CubeMapConvolutionRenderResources cubeMapConvolutionRenderResources
		{
			.textureCubeMapIndex = m_EnvironmentTexture.GetTextureIndex(),
			.outputIrradianceMapIndex = m_IrradianceMapTexture.GetUAVIndex()
		};

		commandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &cubeMapConvolutionRenderResources, 0u);

		commandList->Dispatch(CONVOLUTED_TEXTURE_DIMENSION / 32u, CONVOLUTED_TEXTURE_DIMENSION / 32u, 6u);

		gfx::utils::TransitionResource(commandList.Get(), m_IrradianceMapTexture.GetTextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

		m_ComputeCommandQueue.ExecuteAndFlush(commandList.Get());
	}

	// Run compute shader to generate the prefilter cube map (for specular IBL).
	{
		auto commandList = m_ComputeCommandQueue.GetCommandList();

		gfx::utils::TransitionResource(commandList.Get(), m_PreFilterMapTexture.GetTextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		gfx::utils::SetDescriptorHeaps(commandList.Get(), m_SRV_CBV_UAV_Descriptor);
		gfx::Material::BindRootSignatureCS(commandList.Get());

		m_Materials[L"PreFilterCubeMapMaterial"].BindPSO(commandList.Get());

		uint32_t size{ PREFILTER_TEXTURE_DIMENSION };
		for (uint32_t i = 0; i < m_PreFilterMapTexture.GetMipLevels(); i++)
		{
			m_PreFilterMapTexture.CreateUAV(m_Device.Get(), m_SRV_CBV_UAV_Descriptor, i);

			PreFilterCubeMapRenderResources preFilterCubeMapRenderResources
			{
				.textureCubeMapIndex = m_EnvironmentTexture.GetTextureIndex(),
				.outputPreFilteredCubeMapIndex = m_PreFilterMapTexture.GetUAVIndex(i),
				.mipLevel = i
			};


			const uint32_t numGroups = std::max(1u, size / 32u);

			commandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &preFilterCubeMapRenderResources, 0u);

			commandList->Dispatch(numGroups, numGroups, 6u);
			size /= 2;
		}

		gfx::utils::TransitionResource(commandList.Get(), m_PreFilterMapTexture.GetTextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_ComputeCommandQueue.ExecuteAndFlush(commandList.Get());
	}

	// Run compute shader to generate BRDF Convolution Texture.
	{
		auto commandList = m_ComputeCommandQueue.GetCommandList();

		gfx::utils::SetDescriptorHeaps(commandList.Get(), m_SRV_CBV_UAV_Descriptor);
		gfx::Material::BindRootSignatureCS(commandList.Get());

		gfx::utils::TransitionResource(commandList.Get(), m_BRDFConvolutionTexture.GetTextureResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		m_Materials[L"BRDFConvolutionMaterial"].BindPSO(commandList.Get());

		m_BRDFConvolutionTexture.CreateUAV(m_Device.Get(), m_SRV_CBV_UAV_Descriptor, 0u);

		BRDFConvolutionRenderResources brdfConvolutionRenderResources
		{
			.lutTextureIndex = m_BRDFConvolutionTexture.GetUAVIndex()
		};

		commandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &brdfConvolutionRenderResources, 0u);

		commandList->Dispatch(BRDF_CONVOLUTION_TEXTURE_DIMENSION / 32u, BRDF_CONVOLUTION_TEXTURE_DIMENSION / 32u, 1u);

		gfx::utils::TransitionResource(commandList.Get(), m_BRDFConvolutionTexture.GetTextureResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		
		m_ComputeCommandQueue.ExecuteAndFlush(commandList.Get());
	}
}

void SandBox::RenderLightSources(ID3D12GraphicsCommandList* commandList, gfx::Material& material)
{
	// Draw the light source
	material.BindPSO(commandList);

	LightRenderResources lightRenderResources{};

	for (auto& pointLight : m_Lights)
	{
		if (pointLight.GetLightType() == EnumClassValue(gfx::LightTypes::PointLightData))
		{
			lightRenderResources =
			{
				.lightDataCBufferIndex = gfx::Light::GetLightDataCBufferIndex(),
				.lightIndex = pointLight.GetLightIndex(),
			};

			pointLight.Draw(commandList, lightRenderResources);
		}
	}
}

void SandBox::RenderGameObjects(ID3D12GraphicsCommandList* commandList, gfx::Material& material, bool enableIBL)
{
	material.BindPSO(commandList);

	PBRRenderResources pbrRenderResources
	{
		.cameraCBufferIndex = m_Camera.GetCameraDataCBufferIndex(),
		.lightDataCBufferIndex = gfx::Light::GetLightDataCBufferIndex(),
		.enableIBL = enableIBL,
		.irradianceMap = m_IrradianceMapTexture.GetTextureIndex(),
		.prefilterMap = m_PreFilterMapTexture.GetTextureIndex(),
		.brdfConvolutionLUTMap = m_BRDFConvolutionTexture.GetTextureIndex()
	};

	for (auto& pbrModel : m_PBRModels)
	{
		pbrModel.second.Draw(commandList, pbrRenderResources);
	}
}

void SandBox::RenderGBufferPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, gfx::DescriptorHandle& dsvHandle)
{
	// For reference : 
	// gfx::DescriptorHandle albedoRTV =	m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(0u));
	// gfx::DescriptorHandle positionRTV =	m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(1u));
	// gfx::DescriptorHandle normalRTV =	m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(2u));
	// gfx::DescriptorHandle aoMetalRoughnessRTV = m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(3u));
	// gfx::DescriptorHandle emissiveRTV = m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(4u));

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE , 5u> gPassRTVs
	{
		m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(0u)).cpuDescriptorHandle,
		m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(1u)).cpuDescriptorHandle,
		m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(2u)).cpuDescriptorHandle,
		m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(3u)).cpuDescriptorHandle,
		m_RTVDescriptor.GetDescriptorHandleFromIndex(m_GBuffer.GetRTVIndex(4u)).cpuDescriptorHandle,
	};

	for (uint32_t i = 0; i < gPassRTVs.size(); ++i)
	{
		gfx::utils::TransitionResource(commandList, m_GBuffer.GetResource(i), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	commandList->OMSetRenderTargets(static_cast<UINT>(gPassRTVs.size()), gPassRTVs.data(), TRUE, &dsvHandle.cpuDescriptorHandle);

	static const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (uint32_t i = 0; i < gPassRTVs.size(); ++i)
	{
		commandList->ClearRenderTargetView(gPassRTVs[i], clearColor, 0u, nullptr);
	}

	material.BindPSO(commandList);

	GPassRenderResources gPassRenderResources{};

	for (auto& pbrModel : m_PBRModels)
	{
		pbrModel.second.Draw(commandList, gPassRenderResources);
	}

	// Now that all the RTV's are with the desired geometry data, the scene can be rendered once again (to fill the depth buffer with appropirate values).

	for (uint32_t i = 0; i < gPassRTVs.size(); ++i)
	{
		gfx::utils::TransitionResource(commandList, m_GBuffer.GetResource(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

void SandBox::RenderDeferredPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, bool enableIBL, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle)
{
	material.BindPSO(commandList);

	DeferredPassRenderResources deferredPBRRenderResources
	{
		.albedoGPassSRVIndex = m_GBuffer.GetSRVIndex(0u),
		.positionGPassSRVIndex = m_GBuffer.GetSRVIndex(1u),
		.normalGPassSRVIndex = m_GBuffer.GetSRVIndex(2u),
		.aoMetalRoughnessGPassSRVIndex = m_GBuffer.GetSRVIndex(3u),
		.emissiveGPassSRVIndex = m_GBuffer.GetSRVIndex(4u),
		.cameraCBufferIndex = m_Camera.GetCameraDataCBufferIndex(),
		.lightDataCBufferIndex = gfx::Light::GetLightDataCBufferIndex(),
		.shadowMappingCBufferStartingIndex = m_CSMShadowMappingData[0].GetBufferIndex(),
		.shadowDepthBufferStartingIndex = m_CSMDepthMaps[0].GetSRVIndex(),
		.enableIBL = enableIBL,
		.irradianceMap = m_IrradianceMapTexture.GetTextureIndex(),
		.prefilterMap = m_PreFilterMapTexture.GetTextureIndex(),
		.brdfConvolutionLUTMap = m_BRDFConvolutionTexture.GetTextureIndex(),
		.positionBufferIndex = gfx::RenderTarget::GetPositionBufferIndex(),
		.textureBufferIndex = gfx::RenderTarget::GetTextureCoordsBufferIndex()
	};

	
	auto deferredRtvHandle = m_RTVDescriptor.GetDescriptorHandleFromIndex(m_DeferredRT.GetRTVIndex());
	gfx::utils::TransitionResource(commandList, m_DeferredRT.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto deferredDsvHandle = m_DSVDescriptor.GetDescriptorHandleFromIndex(m_DeferredDepthBuffer.GetBufferIndex());

	gfx::utils::ClearRTV(commandList, deferredRtvHandle, std::vector<float>{0.0f, 0.0f, 0.0f, 1.0f});
	gfx::utils::ClearDepthBuffer(commandList, deferredDsvHandle);

	commandList->OMSetRenderTargets(1u, &deferredRtvHandle.cpuDescriptorHandle, FALSE, &deferredDsvHandle.cpuDescriptorHandle);
	
	gfx::RenderTarget::Bind(commandList);

	commandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &deferredPBRRenderResources, 0u);

	commandList->DrawIndexedInstanced(gfx::RenderTarget::RT_INDICES_COUNT, 1u, 0u, 0u, 0u);

	gfx::utils::TransitionResource(commandList, m_DeferredRT.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Set the RTV and DSV to the rtv / dsv handle passed into the function.
	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);
}

void SandBox::RenderShadowPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle)
{
	material.BindPSO(commandList);

	commandList->RSSetViewports(1u, &m_ShadowViewport);
	auto shadowDepthHandle = m_DSVDescriptor.GetDescriptorHandleFromIndex(m_ShadowDepthBuffer.GetBufferIndex());
	
	gfx::utils::TransitionResource(commandList, m_ShadowDepthBuffer.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	
	gfx::utils::ClearDepthBuffer(commandList, shadowDepthHandle);

	commandList->OMSetRenderTargets(0u, nullptr, TRUE, &shadowDepthHandle.cpuDescriptorHandle);

	ShadowPassRenderResources shadowPassRenderResources
	{
		.shadowMappingCBufferIndex = m_ShadowMappingData.GetBufferIndex()
	};
	
	for (auto& [modelName, model] : m_PBRModels)
	{
		model.Draw(commandList, shadowPassRenderResources);
	}

	gfx::utils::TransitionResource(commandList, m_ShadowDepthBuffer.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	// Set the RTV and DSV to the rtv / dsv handle passed into the function.
	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);

	commandList->RSSetViewports(1u, &m_Viewport);
}

void SandBox::RenderShadowCSMPass(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle)
{
	material.BindPSO(commandList);

	commandList->RSSetViewports(1u, &m_ShadowViewport);
	for (uint32_t i = 0; i < CSM_DEPTH_MAPS; ++i)
	{
		auto shadowDepthHandle = m_DSVDescriptor.GetDescriptorHandleFromIndex(m_CSMDepthMaps[i].GetBufferIndex());

		gfx::utils::TransitionResource(commandList, m_CSMDepthMaps[i].GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		gfx::utils::ClearDepthBuffer(commandList, shadowDepthHandle);

		commandList->OMSetRenderTargets(0u, nullptr, TRUE, &shadowDepthHandle.cpuDescriptorHandle);

		ShadowPassRenderResources shadowPassRenderResources
		{
			.shadowMappingCBufferIndex = m_CSMShadowMappingData[i].GetBufferIndex()
		};

		for (auto& [modelName, model] : m_PBRModels)
		{
			model.Draw(commandList, shadowPassRenderResources);
		}

		gfx::utils::TransitionResource(commandList, m_CSMDepthMaps[i].GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// Set the RTV and DSV to the rtv / dsv handle passed into the function.
	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);

	commandList->RSSetViewports(1u, &m_Viewport);
}

void SandBox::RenderSkyBox(ID3D12GraphicsCommandList* commandList, gfx::Material& material)
{
	material.BindPSO(commandList);

	SkyBoxRenderResources skyBoxRenderResources
	{
		.textureIndex = m_EnvironmentTexture.GetTextureIndex()
	};

	m_SkyBoxModel.Draw(commandList, skyBoxRenderResources);
}

void SandBox::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList, helios::gfx::Material& material, helios::gfx::DescriptorHandle& rtvHandle, helios::gfx::DescriptorHandle& dsvHandle)
{
	material.BindPSO(commandList);

	gfx::RenderTarget::Bind(commandList);
	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);

	RenderTargetRenderResources bindlessRenderResource
	{
		.positionBufferIndex = gfx::RenderTarget::GetPositionBufferIndex(),
		.textureBufferIndex = gfx::RenderTarget::GetTextureCoordsBufferIndex(),
		.textureIndex = m_OffscreenRT.GetSRVIndex(),
		.deferredPassTextureIndex = m_DeferredRT.GetSRVIndex(),
		.renderTargetSettingsCBufferIndex = m_RenderTargetSettingsData.GetBufferIndex()
	};

	commandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &bindlessRenderResource, 0u);

	commandList->DrawIndexedInstanced(gfx::RenderTarget::RT_INDICES_COUNT, 1u, 0u, 0u, 0u);
}

std::array<DirectX::XMVECTOR, 8> SandBox::GetFrustumCorners(DirectX::XMMATRIX& projectionMatrix)
{
	dx::XMMATRIX inverseViewProjection = (dx::XMMatrixInverse(nullptr, m_Camera.GetViewMatrix() * (projectionMatrix)));

	std::array<dx::XMVECTOR, 8> frustrumCorners{};
	uint32_t cornerIndex{};
	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 2; ++y)
		{
			for (int z = 0; z < 2; ++z)
			{
				dx::XMVECTOR point = dx::XMVector4Transform(dx::XMVectorSet(2.0f * x - 1.0f, 2.0f * y - 1.0f, (float)z, 1.0f), inverseViewProjection);
				point = dx::XMVectorSet(dx::XMVectorGetX(point) / dx::XMVectorGetW(point), dx::XMVectorGetY(point) / dx::XMVectorGetW(point), dx::XMVectorGetZ(point) / dx::XMVectorGetW(point), 1.0f);
				frustrumCorners[cornerIndex++] = point;
			}
		}
	}

	return frustrumCorners;
}

std::pair<DirectX::XMMATRIX, DirectX::XMMATRIX> SandBox::GetLightSpaceMatrix(float nearPlane, float farPlane)
{
	dx::XMMATRIX projectionMatrix = dx::XMMatrixPerspectiveFovLH(m_FOV, m_AspectRatio, nearPlane, farPlane);
	const auto frustrumCorners = GetFrustumCorners(projectionMatrix);

	// Get the center of the frustrum.
	dx::XMVECTOR frustrumCenter{ dx::XMVectorZero() };
	for (const auto& corner : frustrumCorners)
	{
		frustrumCenter = dx::XMVectorSet(dx::XMVectorGetX(frustrumCenter) + dx::XMVectorGetX(corner), dx::XMVectorGetY(frustrumCenter) + dx::XMVectorGetY(corner), dx::XMVectorGetZ(frustrumCenter) + dx::XMVectorGetZ(corner), 1.0f);
	}

	frustrumCenter = dx::XMVectorSet(dx::XMVectorGetX(frustrumCenter) / 8.0f, dx::XMVectorGetY(frustrumCenter) / 8.0f, dx::XMVectorGetZ(frustrumCenter) / 8.0f, 1.0f);

	dx::XMVECTOR upVector = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	dx::XMVECTOR lightDirection = dx::XMLoadFloat4(&gfx::Light::GetLightData().lightPosition[0]);
	m_CSMLightViewMatrix = dx::XMMatrixLookAtLH(frustrumCenter + lightDirection, frustrumCenter, upVector);

	// Finding AABB of the orthographics projection so that it fits tightly about the frustrum (in light view space).
	// Main resource https://learnopengl.com/Guest-Articles/2021/CSM	
	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::min();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::min();

	for (const auto& corner : frustrumCorners)
	{
		dx::XMVECTOR lightViewSpaceCorner = dx::XMVector4Transform(corner, m_CSMLightViewMatrix);
		minX = std::min(minX, dx::XMVectorGetX(lightViewSpaceCorner));
		maxX = std::max(maxX, dx::XMVectorGetX(lightViewSpaceCorner));
		minY = std::min(minY, dx::XMVectorGetY(lightViewSpaceCorner));
		maxY = std::max(maxY, dx::XMVectorGetY(lightViewSpaceCorner));
		minZ = std::min(minZ, dx::XMVectorGetZ(lightViewSpaceCorner));
		maxZ = std::max(maxZ, dx::XMVectorGetZ(lightViewSpaceCorner));
	}

	minZ = minZ < 0.0f ? minZ *= m_ZMultiplier : minZ /= m_ZMultiplier;
	maxZ = maxZ < 0.0f ? maxZ /= m_ZMultiplier : maxX *= m_ZMultiplier;

	m_CSMLightProjectionMatrix = dx::XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, minZ, maxZ);
	return { dx::XMMatrixTranspose(m_CSMLightViewMatrix), dx::XMMatrixTranspose(m_CSMLightProjectionMatrix)};
}

void SandBox::CalculateCSMMatrices()
{
	for (uint32_t i = 0; i < CSM_DEPTH_MAPS; ++i)
	{
		m_CSMLightMatrices[i] = GetLightSpaceMatrix(CSM_CASCADES[i], CSM_CASCADES[i + 1]);
	}
}

void SandBox::CreateFactory()
{
	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(::CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)));
}

void SandBox::EnableDebugLayer()
{
#ifdef _DEBUG
	ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&m_DebugInterface)));
	m_DebugInterface->EnableDebugLayer();
	m_DebugInterface->SetEnableGPUBasedValidation(TRUE);
	m_DebugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);

	// Currently set to default behaviour.
	m_DebugInterface->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
#endif
}

void SandBox::SelectAdapter()
{
	// Index 0 will be the GPU with highest preference.
	ThrowIfFailed(m_Factory->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_Adapter)));
	
#ifdef _DEBUG
	DXGI_ADAPTER_DESC adapterDesc{};
	ThrowIfFailed(m_Adapter->GetDesc(&adapterDesc));
	std::wstring adapterInfo = L"Adapter Description : " + std::wstring(adapterDesc.Description) + L".\n";
	OutputDebugString(adapterInfo.c_str());
#endif
}

void SandBox::CreateDevice()
{
	ThrowIfFailed(::D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));
	m_Device->SetName(L"D3D12 Device");

	// Set break points on certain severity levels in debug mode.
#ifdef _DEBUG
	wrl::ComPtr<ID3D12InfoQueue> infoQueue;
	ThrowIfFailed(m_Device.As(&infoQueue));

	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	// Configure queue filter to ignore info message severity.
	std::array<D3D12_MESSAGE_SEVERITY, 1> ignoreMessageSeverities
	{
		D3D12_MESSAGE_SEVERITY_INFO
	};

	// Configure queue filter to ignore individual messages using thier ID.
	std::array<D3D12_MESSAGE_ID, 2> ignoreMessageIDs
	{
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
	};

	D3D12_INFO_QUEUE_FILTER infoQueueFilter
	{
		.DenyList
		{
			.NumSeverities = static_cast<UINT>(ignoreMessageSeverities.size()),
			.pSeverityList = ignoreMessageSeverities.data(),
			.NumIDs = static_cast<UINT>(ignoreMessageIDs.size()),
			.pIDList = ignoreMessageIDs.data()
		},
	};

	ThrowIfFailed(infoQueue->PushStorageFilter(&infoQueueFilter));

	ThrowIfFailed(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugDevice)));
#endif
}

void SandBox::CheckTearingSupport()
{
	BOOL allowTearing = TRUE;
	wrl::ComPtr<IDXGIFactory5> dxgiFactory;
	
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
	if (FAILED(dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
	{
		allowTearing = FALSE;
	}

	m_IsTearingSupported = allowTearing;
}

void SandBox::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc
	{
		.Width = m_Width,
		.Height = m_Height,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.Stereo = FALSE,
		.SampleDesc
		{
			.Count = 1,
			.Quality = 0,
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = NUMBER_OF_FRAMES,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = m_VSync ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL : DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0u
	};

	wrl::ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(m_Factory->CreateSwapChainForHwnd(m_CommandQueue.GetCommandQueue().Get(), Application::GetWindowHandle(), &swapChainDesc, nullptr, nullptr, &swapChain1));
	
	// Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
	ThrowIfFailed(m_Factory->MakeWindowAssociation(Application::GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));
	
	ThrowIfFailed(swapChain1.As(&m_SwapChain));

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void SandBox::CreateBackBufferRenderTargetViews()
{
	for (int i = 0; i < NUMBER_OF_FRAMES; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle;
		
		wrl::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_BackBuffers[i] = backBuffer;
		m_BackBuffers[i]->SetName(L"SwapChain BackBuffer");

		m_RTVDescriptor.OffsetCurrentHandle();
	}
}
