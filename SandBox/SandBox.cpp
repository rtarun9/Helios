#include "Pch.hpp"

#include "SandBox.hpp"

using namespace helios;

struct LightingData
{
	dx::XMFLOAT4 lightPosition;
	dx::XMVECTOR cameraPosition;
};

enum class ShaderRegisterSpace : uint32_t
{
	VertexShader = 0u,
	PixelShader = 1u
};

enum class RootParameterIndex : uint32_t
{
	PositionBuffer = 0u,
	TextureCoordBuffer = 1u,
	NormalBuffer = 2u,
	ConstantBuffer = 3u,
	RootConstant = 4u,
	DescriptorTable = 5u,
	ParamterCount = 6u
};

enum class PBRRootParameterIndex : uint32_t
{
	PositionBuffer = 0u,
	TextureCoordBuffer = 1u,
	NormalBuffer = 2u,
	VertexConstantBuffer = 3u,
	PixelConstantBuffer = 4u,
	PixelRootConstant = 5u,
	DescriptorTable = 6u,
	ParamterCount = 7u
};

enum class SkyBoxParamterIndex : uint32_t
{
	PositionBuffer = 0u,
	TextureCoordBuffer = 1u,
	VertexConstantBuffer = 2u,
	DescriptorTable = 3u,
	ParamterCount = 4u
};

enum class RenderTargetRootParamterIndex : uint32_t
{
	PositionBuffer = 0u,
	TextureCoordBuffer = 1u,
	DescriptorTable = 2u,
	ParamterCount = 3u
};

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

	m_ViewMatrix = m_Camera.GetViewMatrix();

	m_ProjectionMatrix = dx::XMMatrixPerspectiveFovLH(dx::XMConvertToRadians(m_FOV), m_AspectRatio, 0.1f, 1000.0f);
	
	m_LightSource.GetTransform().translate.x = 2.0f;
	m_LightSource.GetTransform().translate.y = static_cast<float>(sin(Application::GetTimer().GetTotalTime() / 2.0f));

	m_LightSource.GetTransform().scale = dx::XMFLOAT3(0.1f, 0.1f, 0.1f);

	m_PBRMaterial.Update();
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
	if (isKeyDown)
	{
		if (keycode == VK_SPACE)
		{
			m_FOV -= static_cast<float>(Application::GetTimer().GetDeltaTime() * 10);
		}
	}

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
	}
}


void SandBox::PopulateCommandList(ID3D12GraphicsCommandList* commandList, ID3D12Resource* currentBackBuffer)
{
	m_UIManager.FrameStart();

	auto projectionView = m_ViewMatrix * m_ProjectionMatrix;

	// TODO : Move to OnUpdate soon.
	for (auto& [objectName, gameObject] : m_GameObjects)
	{
		gameObject.UpdateData(objectName);
		gameObject.UpdateTransformData(commandList, projectionView);
	}

	m_Sphere.UpdateData(L"Sphere");
	m_Sphere.UpdateTransformData(commandList, projectionView);

	ImGui::Begin("Material Data");
	ImGui::SliderFloat3("Albedo", &m_PBRMaterial.GetBufferData().albedo.x, 0.0f, 1.0f);
	ImGui::SliderFloat("Metallic Factor", &m_PBRMaterial.GetBufferData().metallicFactor, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness Factor", &m_PBRMaterial.GetBufferData().roughnessFactor, 0.0f, 1.0f);
	ImGui::End();

	m_LightSource.UpdateData(L"Light Source");
	m_LightSource.UpdateTransformData(commandList, projectionView);

	// Set the necessary states
	m_Materials[L"DefaultMaterial"].Bind(commandList);

	std::array<ID3D12DescriptorHeap*, 1> descriptorHeaps
	{
		m_SRV_CBV_UAV_Descriptor.GetDescriptorHeap()
	};

	commandList->SetDescriptorHeaps(static_cast<uint32_t>(descriptorHeaps.size()), descriptorHeaps.data());

	commandList->RSSetViewports(1u, &m_Viewport);
	commandList->RSSetScissorRects(1u, &m_ScissorRect);

	// Inidicate render target will be used as RTV.
	gfx::utils::TransitionResource(commandList, m_OffscreenRT.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Record rendering commands
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_OffscreenRT.GetRTVCPUDescriptorHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DSVDescriptor.GetCPUDescriptorHandleForStart();

	m_UIManager.Begin(L"Scene Settings");
	static std::array<float, 4> clearColor{ 0.10f, 0.10f, 0.1f, 1.0f };
	m_UIManager.SetClearColor(clearColor);

	gfx::utils::ClearRTV(commandList, rtvHandle, clearColor);

	gfx::utils::ClearDepthBuffer(commandList, dsvHandle);

	commandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	LightingData lightingData
	{
		.lightPosition = {m_LightSource.GetTransform().translate.x, m_LightSource.GetTransform().translate.y, m_LightSource.GetTransform().translate.z, 1.0f},
		.cameraPosition = m_Camera.m_CameraPosition
	};

	for (auto& [objectName, gameObject] : m_GameObjects)
	{
		auto textureGPUHandle = gameObject.GetTexture().m_BaseColorDescriptorHandle;
		if (!textureGPUHandle.ptr)
		{
			textureGPUHandle = m_Textures[L"TestTexture"].GetGPUDescriptorHandle();
		}

		commandList->SetGraphicsRootShaderResourceView(EnumClassValue(RootParameterIndex::PositionBuffer), gameObject.GetPositionBuffer()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootShaderResourceView(EnumClassValue(RootParameterIndex::TextureCoordBuffer), gameObject.GetTextureCoordsBuffer()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootShaderResourceView(EnumClassValue(RootParameterIndex::NormalBuffer), gameObject.GetNormalBuffer()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(EnumClassValue(RootParameterIndex::DescriptorTable), textureGPUHandle);
		commandList->SetGraphicsRootConstantBufferView(EnumClassValue(RootParameterIndex::ConstantBuffer), gameObject.GetTransformCBufferVirtualAddress());
		commandList->SetGraphicsRoot32BitConstants(EnumClassValue(RootParameterIndex::RootConstant), sizeof(LightingData) / 4, &lightingData, 0u);

		gameObject.Draw(commandList);
	}
	
	// Draw sphere (for PBR Test).
	m_Materials[L"PBRMaterial"].Bind(commandList);

	std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> pbrMaterialDescriptorHandles
	{
		m_Textures[L"SphereAlbedoTexture"].GetGPUDescriptorHandle(),
		m_Textures[L"SphereMetalRoughTexture"].GetGPUDescriptorHandle()
	};

	auto textureGPUHandle = m_Textures[L"TestTexture"].GetGPUDescriptorHandle();

	auto pbrMaterialGPUVirutalAddress = m_PBRMaterial.GetBufferView().BufferLocation;
	commandList->SetGraphicsRootShaderResourceView(EnumClassValue(PBRRootParameterIndex::PositionBuffer), m_Sphere.GetPositionBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootShaderResourceView(EnumClassValue(PBRRootParameterIndex::TextureCoordBuffer), m_Sphere.GetTextureCoordsBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootShaderResourceView(EnumClassValue(PBRRootParameterIndex::NormalBuffer), m_Sphere.GetNormalBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(EnumClassValue(PBRRootParameterIndex::DescriptorTable), m_Textures[L"SphereAlbedoTexture"].GetGPUDescriptorHandle());
	commandList->SetGraphicsRootConstantBufferView(EnumClassValue(PBRRootParameterIndex::VertexConstantBuffer), m_Sphere.GetTransformCBufferVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(EnumClassValue(PBRRootParameterIndex::PixelConstantBuffer), pbrMaterialGPUVirutalAddress);
	commandList->SetGraphicsRoot32BitConstants(EnumClassValue(PBRRootParameterIndex::PixelRootConstant), sizeof(LightingData) / 4, &lightingData, 0u);

	m_Sphere.Draw(commandList);

	// Draw the light source
	m_Materials[L"LightMaterial"].Bind(commandList);

	m_LightSource.UpdateTransformData(commandList, projectionView);
	commandList->SetGraphicsRootShaderResourceView(0u, m_LightSource.GetPositionBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootShaderResourceView(1u, m_LightSource.GetTextureCoordsBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootShaderResourceView(2u, m_LightSource.GetNormalBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(3u, m_LightSource.GetTransformCBufferVirtualAddress());

	m_LightSource.Draw(commandList);

	m_UIManager.End();

	gfx::utils::TransitionResource(commandList, m_OffscreenRT.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	gfx::utils::TransitionResource(commandList, currentBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_Materials[L"OffscreenRTMaterial"].Bind(commandList);

	rtvHandle = m_RTVDescriptor.GetCPUDescriptorHandleForStart();
	m_RTVDescriptor.Offset(rtvHandle, m_CurrentBackBufferIndex);

	commandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);
	gfx::RenderTarget::Bind(commandList);

	commandList->SetGraphicsRootDescriptorTable(EnumClassValue(RenderTargetRootParamterIndex::DescriptorTable), m_OffscreenRT.GetSRVGPUDescriptorHandle());

	commandList->SetGraphicsRootShaderResourceView(EnumClassValue(RenderTargetRootParamterIndex::PositionBuffer), gfx::RenderTarget::GetPositionBuffer()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootShaderResourceView(EnumClassValue(RenderTargetRootParamterIndex::TextureCoordBuffer), gfx::RenderTarget::GetTextureCoordsBuffer()->GetGPUVirtualAddress());
	
	commandList->DrawIndexedInstanced(6u, 1u, 0u, 0u, 0u);

	m_UIManager.FrameEnd(commandList);

	gfx::utils::TransitionResource(commandList, currentBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

}

void SandBox::InitRendererCore()
{
	EnableDebugLayer();
	SelectAdapter();
	CreateDevice();

	m_CommandQueue.Init(m_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Main Command Queue");

	CheckTearingSupport();
	CreateSwapChain();

	m_RTVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"RTV Descriptor");

	m_DSVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1u, L"DSV Descriptor");

	// Creating 15 descriptor heap slots as of now, just an arbitruary number.
	m_SRV_CBV_UAV_Descriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 20u, L"SRV_CBV_UAV Descriptor");

	CreateBackBufferRenderTargetViews();
	CreateDepthBuffer();

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
	// Reset command list and allocator for initial setup.
	auto commandList = m_CommandQueue.GetCommandList();

	LoadMaterials();
	LoadTextures(commandList.Get());
	LoadModels(commandList.Get());

	// Load data for models and textures.
	m_PBRMaterial.Init(m_Device.Get(), commandList.Get(), MaterialData{ .albedo = dx::XMFLOAT3(1.0f, 1.0f, 1.0f), .roughnessFactor = 0.1f }, m_SRV_CBV_UAV_Descriptor, L"Material PBR CBuffer");

	// Create render targets and thier Root signature and PSO.
	gfx::RenderTarget::InitBuffers(m_Device.Get(), commandList.Get());

	m_OffscreenRT.Init(m_Device.Get(), commandList.Get(), DXGI_FORMAT_R16G16B16A16_FLOAT, m_RTVDescriptor, m_SRV_CBV_UAV_Descriptor, m_Width, m_Height, L"Offscreen RT");

	// Close command list and execute it (for the initial setup).
	m_FrameFenceValues[m_CurrentBackBufferIndex] =  m_CommandQueue.ExecuteCommandList(commandList.Get());
	m_CommandQueue.FlushQueue();
}

void SandBox::LoadMaterials()
{
	m_Materials[L"DefaultMaterial"] = gfx::utils::CreateMaterial(m_Device.Get(), L"Shaders/TestVS.cso", L"Shaders/TestPS.cso", L"Default Material");
	m_Materials[L"LightMaterial"] = gfx::utils::CreateMaterial(m_Device.Get(), L"Shaders/LightVS.cso", L"Shaders/LightPS.cso", L"Light Material");
	m_Materials[L"PBRMaterial"] = gfx::utils::CreateMaterial(m_Device.Get(), L"Shaders/PBRVS.cso", L"Shaders/PBRPS.cso", L"PBR Material");
	m_Materials[L"OffscreenRTMaterial"] = gfx::utils::CreateMaterial(m_Device.Get(), L"Shaders/OffscreenRTVS.cso", L"Shaders/OffscreenRTPS.cso", L"Offscreen RT", DXGI_FORMAT_R8G8B8A8_UNORM);
}

void SandBox::LoadTextures(ID3D12GraphicsCommandList* commandList)
{
	m_Textures[L"TestTexture"].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, L"Assets/Textures/TestTexture.png", L"Test Texture");
	m_Textures[L"MarbleTexture"].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, L"Assets/Textures/Marble.jpg", L"Marble Texture");
	m_Textures[L"SphereAlbedoTexture"].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, L"Assets/Models/MetalRoughSpheres/glTF/Spheres_BaseColor.png", L"Sphere Base Color Texture", true);
	m_Textures[L"SphereMetalRoughTexture"].Init(m_Device.Get(), commandList, m_SRV_CBV_UAV_Descriptor, L"Assets/Models/MetalRoughSpheres/glTF/Spheres_MetalRough.png", L"Sphere Roughness Metallic Texture", false);
}

void SandBox::LoadModels(ID3D12GraphicsCommandList* commandList)
{
	m_GameObjects[L"Cube"].Init(m_Device.Get(), commandList, L"Assets/Models/Cube/Cube.gltf", m_SRV_CBV_UAV_Descriptor, Texture{ .m_BaseColorDescriptorHandle = m_Textures[L"TestTexture"].GetGPUDescriptorHandle() });
	m_GameObjects[L"Cube"].GetTransform().translate = dx::XMFLOAT3(0.0f, 5.0f, 0.0f);

	m_GameObjects[L"Floor"].Init(m_Device.Get(), commandList, L"Assets/Models/Cube/Cube.gltf", m_SRV_CBV_UAV_Descriptor, Texture{ .m_BaseColorDescriptorHandle = m_Textures[L"MarbleTexture"].GetGPUDescriptorHandle() });
	m_GameObjects[L"Floor"].GetTransform().translate = dx::XMFLOAT3(0.0f, -2.0f, 0.0f);
	m_GameObjects[L"Floor"].GetTransform().scale = dx::XMFLOAT3(10.0f, 0.1f, 10.0f);

	m_LightSource.Init(m_Device.Get(), commandList, L"Assets/Models/Cube/Cube.gltf", m_SRV_CBV_UAV_Descriptor);
	m_LightSource.GetTransform().scale = dx::XMFLOAT3(0.1f, 0.1f, 0.1f);

	m_Sphere.Init(m_Device.Get(), commandList, L"Assets/Models/Sphere/scene.gltf", m_SRV_CBV_UAV_Descriptor);
	m_Sphere.GetTransform().scale = dx::XMFLOAT3(0.2f, 0.2f, 0.2f);
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
	wrl::ComPtr<IDXGIFactory6> dxgiFactory;
	UINT createFactoryFlags = 0;

#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	// Index 0 will be the GPU with highest preference.
	ThrowIfFailed(dxgiFactory->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_Adapter)));
	
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
	std::array<D3D12_MESSAGE_ID, 1> ignoreMessageIDs
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
	wrl::ComPtr<IDXGIFactory7> dxgiFactory;
	UINT createFactoryFlags = 0;

#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

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
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_CommandQueue.GetCommandQueue().Get(), Application::GetWindowHandle(), &swapChainDesc, nullptr, nullptr, &swapChain1));
	
	// Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(Application::GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));
	
	ThrowIfFailed(swapChain1.As(&m_SwapChain));

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

void SandBox::CreateBackBufferRenderTargetViews()
{
	for (int i = 0; i < NUMBER_OF_FRAMES; ++i)
	{
		wrl::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_RTVDescriptor.GetCurrentCPUDescriptorHandle());

		m_BackBuffers[i] = backBuffer;
		m_BackBuffers[i]->SetName(L"SwapChain BackBuffer");

		m_RTVDescriptor.OffsetCurrentCPUDescriptor();
	}
}

void SandBox::CreateDepthBuffer()
{
	D3D12_CLEAR_VALUE clearValue
	{
		.Format = DXGI_FORMAT_D32_FLOAT,
		.DepthStencil
		{
			.Depth = 1.0f,
			.Stencil = 0u
		}
	};

	CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthTextureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_Width, m_Height, 1u, 0u, 1u, 0u, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(m_Device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &depthTextureResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&m_DepthBuffer)));

	// Create DSV for the texture
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
	{
		.Format = DXGI_FORMAT_D32_FLOAT,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D
		{
			.MipSlice = 0u,
		}
	};

	m_Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DSVDescriptor.GetCurrentCPUDescriptorHandle());
	m_DSVDescriptor.OffsetCurrentCPUDescriptor();
}

