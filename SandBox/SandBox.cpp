#include "Pch.hpp"
#include "SandBox.hpp"

#include "BindlessRS.hlsli" 

using namespace helios;

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

	m_PBRMaterial.Update();

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

		m_DepthBuffer.Init(m_Device.Get(), m_DSVDescriptor, m_Width, m_Height, L"Depth Buffer");
	}
}

void SandBox::PopulateCommandList(ID3D12GraphicsCommandList* commandList, ID3D12Resource* currentBackBuffer)
{
	m_UIManager.FrameStart();
	
	m_UIManager.Begin(L"Scene Control");

	if (ImGui::TreeNode("Material Data"))
	{
		ImGui::SliderFloat3("Albedo", &m_PBRMaterial.GetBufferData().albedo.x, 0.0f, 1.0f);
		ImGui::SliderFloat("Metallic Factor", &m_PBRMaterial.GetBufferData().metallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness Factor", &m_PBRMaterial.GetBufferData().roughnessFactor, 0.0f, 1.0f);
		ImGui::TreePop();
	}
	
	if (ImGui::TreeNode("Render Target Settings"))
	{
		ImGui::SliderFloat("Exposure", &m_RenderTargetSettingsData.GetBufferData().exposure, 0.1f, 5.0f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Camera Speed"))
	{
		ImGui::SliderFloat("Camera Speed", &m_Camera.m_MovementSpeed, 0.0f, 1250.0f);
		ImGui::TreePop();
	}

	// Test code.
	static bool enableIBL{ true };
	if (ImGui::TreeNode("Enable IBL"))
	{
		ImGui::Checkbox("Enable IBL", &enableIBL);
		ImGui::TreePop(); 
	}

	for (auto& model : m_PBRModels)
	{
		model.second.UpdateTransformData(commandList, m_ProjectionMatrix, m_ViewMatrix);
	}

	for (auto& pointLight : m_PointLights)
	{
		pointLight.UpdateTransformData(commandList, m_ProjectionMatrix, m_ViewMatrix);
	}

	m_SkyBoxModel.UpdateTransformData(commandList, m_ProjectionMatrix, m_ViewMatrix);
	
	static std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	m_UIManager.SetClearColor(clearColor);

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

	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);
	
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Rendering commands

	// Set descriptor heaps and root signature once per command list.
	gfx::utils::SetDescriptorHeaps(commandList, m_SRV_CBV_UAV_Descriptor);
	gfx::Material::BindRootSignature(commandList);

	// Draw the light source
	m_Materials[L"LightMaterial"].BindPSO(commandList);

	LightRenderResources lightRenderResources{};

	for (auto& pointLight : m_PointLights)
	{
		lightRenderResources =
		{
			.pointLightCBufferIndex = gfx::PointLight::GetLightDataCBufferIndex(),
			.pointLightIndex = pointLight.GetPointLightIndex()
		};

		pointLight.Draw(commandList, lightRenderResources);
	}

	// Draw PBR materials.
	m_Materials[L"PBRMaterial"].BindPSO(commandList);

	PBRRenderResources pbrRenderResources
	{
		.materialCBufferIndex = m_PBRMaterial.GetBufferIndex(),
		.cameraCBufferIndex = m_Camera.GetCameraDataCBufferIndex(),
		.pointLightCBufferIndex = gfx::PointLight::GetLightDataCBufferIndex(),
		.enableIBL = enableIBL,
		.irradianceMap = m_IrradianceMapTexture.GetTextureIndex(),
		.prefilterMap = m_PreFilterMapTexture.GetTextureIndex(),
		.brdfConvolutionLUTMap = m_BRDFConvolutionTexture.GetTextureIndex()
	};

	for (auto& pbrModel : m_PBRModels)
	{
		pbrModel.second.Draw(commandList, pbrRenderResources);
	}

	// Draw sky box
	m_Materials[L"SkyBoxMaterial"].BindPSO(commandList);

	SkyBoxRenderResources skyBoxRenderResources
	{
		.textureIndex = m_EnvironmentTexture.GetTextureIndex()
	};

	m_SkyBoxModel.Draw(commandList, skyBoxRenderResources);

	m_UIManager.End();

	gfx::utils::TransitionResource(commandList, m_OffscreenRT.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	gfx::utils::TransitionResource(commandList, currentBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	rtvHandle = m_RTVDescriptor.GetDescriptorHandleForStart();
	m_RTVDescriptor.Offset(rtvHandle, m_CurrentBackBufferIndex);

	commandList->OMSetRenderTargets(1u, &rtvHandle.cpuDescriptorHandle, FALSE, &dsvHandle.cpuDescriptorHandle);
	gfx::RenderTarget::Bind(commandList);

	m_Materials[L"OffscreenRTMaterial"].BindPSO(commandList);

	RenderTargetRenderResources bindlessRenderResource
	{
		.positionBufferIndex = gfx::RenderTarget::GetPositionBufferIndex(),
		.textureBufferIndex = gfx::RenderTarget::GetTextureCoordsBufferIndex(),
		.textureIndex = m_OffscreenRT.GetSRVIndex(),
		.renderTargetSettingsCBufferIndex = m_RenderTargetSettingsData.GetBufferIndex()
	};

	commandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_ROOTCONSTANTS, &bindlessRenderResource, 0u);

	commandList->DrawIndexedInstanced(gfx::RenderTarget::RT_INDICES_COUNT, 1u, 0u, 0u, 0u);

	m_UIManager.FrameEnd(commandList);

	gfx::utils::TransitionResource(commandList, currentBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void SandBox::InitRendererCore()
{
	CreateFactory();
	EnableDebugLayer();
	SelectAdapter();
	CreateDevice();

	m_CommandQueue.Init(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Main Command Queue");
	m_ComputeCommandQueue.Init(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Compute Command Queue");

	CheckTearingSupport();
	CreateSwapChain();

	m_RTVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"RTV Descriptor");

	m_DSVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 5u, L"DSV Descriptor");

	m_SRV_CBV_UAV_Descriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1000u, L"SRV_CBV_UAV Descriptor");

	CreateBackBufferRenderTargetViews();

	m_DepthBuffer.Init(m_Device.Get(), m_DSVDescriptor, m_Width, m_Height, L"Depth Stencil Buffer");

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
	LoadRenderTargets(commandList.Get());

	// Close command list and execute it (for the initial setup).
	m_FrameFenceValues[m_CurrentBackBufferIndex] =  m_CommandQueue.ExecuteCommandList(commandList.Get());
	m_CommandQueue.FlushQueue();
	
	cubeMapThread.join();
}

void SandBox::LoadMaterials()
{
	m_Materials[L"LightMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/LightVS.cso", L"Shaders/LightPS.cso", L"Light Material");
	m_Materials[L"PBRMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/PBRVS.cso", L"Shaders/PBRPS.cso", L"PBR Material");
	m_Materials[L"OffscreenRTMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/OffscreenRTVS.cso", L"Shaders/OffscreenRTPS.cso", L"Offscreen RT", DXGI_FORMAT_R8G8B8A8_UNORM);

	m_Materials[L"SkyBoxMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/SkyBoxVS.cso", L"Shaders/SkyBoxPS.cso", L"Sky Box Material", DXGI_FORMAT_R16G16B16A16_FLOAT, true);
	
	m_Materials[L"EquirectEnvironmentMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/CubeFromEquirectTextureCS.cso", L"Cube From Equirect Material");
	m_Materials[L"CubeMapConvolutionMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/CubeMapConvolutionCS.cso", L"Cube Map Convolution Material");
	m_Materials[L"PreFilterCubeMapMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/PreFilterCubeMapCS.cso", L"Pre Filter Cube Map");

	m_Materials[L"BRDFConvolutionMaterial"] = gfx::Material::CreateMaterial(m_Device.Get(), L"Shaders/BRDFConvolutionCS.cso", L"BRDF Convolution Material");
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

	m_PointLights.resize(TOTAL_POINT_LIGHTS);
	gfx::PointLight::InitLightDataCBuffer(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor);

	for (uint32_t i = 0; i < TOTAL_POINT_LIGHTS; ++i)
	{
		m_PointLights[i].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, i);
	}

	m_PBRModels[L"Spheres"].Init(m_Device.Get(), commandList, L"Assets/Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", m_SRV_CBV_UAV_Descriptor, L"Spheres");
	m_PBRModels[L"Spheres"].GetTransform().translate = { 10.0f, 0.0f, 0.0f };

	m_PBRModels[L"SciFi Helmet"].Init(m_Device.Get(), commandList, L"Assets/Models/SciFiHelmet/glTF/SciFiHelmet.gltf", m_SRV_CBV_UAV_Descriptor, L"SciFi Helmet");

	m_PBRModels[L"Damaged Helmet"].Init(m_Device.Get(), commandList, L"Assets/Models/DamagedHelmet/glTF/DamagedHelmet.gltf", m_SRV_CBV_UAV_Descriptor, L"Damaged Helmet");

	//m_PBRModels[L"Sponza"].Init(m_Device.Get(), commandList, L"Assets/Models/Sponza/glTF/Sponza.gltf", m_SRV_CBV_UAV_Descriptor, L"Sponza");
	//m_PBRModels[L"Sponza"].GetTransform().scale = { 0.1f, 0.1f, 0.1f };

	m_PBRMaterial.Init(m_Device.Get(), commandList, MaterialData{ .albedo = dx::XMFLOAT3(1.0f, 0.0f, 0.0f), .roughnessFactor = 0.1f }, m_SRV_CBV_UAV_Descriptor, L"Material PBR CBuffer");
}

void SandBox::LoadRenderTargets(ID3D12GraphicsCommandList* commandList)
{
	// Create render targets and thier Root signature and PSO.
	gfx::RenderTarget::InitBuffers(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor);

	m_OffscreenRT.Init(m_Device.Get(), commandList, DXGI_FORMAT_R16G16B16A16_FLOAT, m_RTVDescriptor, m_SRV_CBV_UAV_Descriptor, m_Width, m_Height, L"Offscreen RT");

	m_RenderTargetSettingsData.Init(m_Device.Get(), commandList, RenderTargetSettings{ .exposure = 1.0f }, m_SRV_CBV_UAV_Descriptor, L"Render Target Settings Data");
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
