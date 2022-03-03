#include "Pch.hpp"

#include "SandBox.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace helios
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texCoord;
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

		float angle = static_cast<float>(Application::GetTimer().GetTotalTime()) * 40.0f;

		static dx::XMVECTOR rotationAxis = dx::XMVectorSet(0.0f, 1.0f, 1.0f, 0.0f);
		m_ModelMatrix = dx::XMMatrixRotationAxis(rotationAxis, dx::XMConvertToRadians(angle));

		static const dx::XMVECTOR EYE_POSITION = dx::XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
		static const dx::XMVECTOR TARGET_POSITION = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		static const dx::XMVECTOR UP_DIRECTION = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		m_ViewMatrix = m_Camera.GetViewMatrix();

		m_ProjectionMatrix = dx::XMMatrixPerspectiveFovLH(dx::XMConvertToRadians(m_FOV), m_AspectRatio, 0.1f, 100.0f);
	}

	void SandBox::OnRender()
	{
		wrl::ComPtr<ID3D12CommandAllocator> commandAllocator = m_CommandAllocator[m_CurrentBackBufferIndex];
		wrl::ComPtr<ID3D12Resource> currentBackBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

		ThrowIfFailed(commandAllocator->Reset());

		ThrowIfFailed(m_CommandList->Reset(commandAllocator.Get(), m_PSO.Get()));

		// Set the necessary states
	
		m_CommandList->SetPipelineState(m_PSO.Get());
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		
		std::array descriptorHeaps
		{
			m_SRVDescriptor.GetDescriptorHeap()
		};
		
		m_CommandList->SetDescriptorHeaps(static_cast<uint32_t>(descriptorHeaps.size()), descriptorHeaps.data());
		m_CommandList->SetGraphicsRootDescriptorTable(0u, m_SRVDescriptor.GetGPUDescriptorHandle());
		
		m_CommandList->RSSetViewports(1u, &m_Viewport);
		m_CommandList->RSSetScissorRects(1u, &m_ScissorRect);
		

		// Inidicate back buffer will be used as RTV.
		gfx::utils::TransitionResource(m_CommandList.Get(), currentBackBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Record rendering commands
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptor.GetCPUDescriptorHandle(), m_CurrentBackBufferIndex, m_RTVDescriptor.GetDescriptorSize());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(m_DSVDescriptor.GetCPUDescriptorHandle());

		gfx::utils::ClearRTV(m_CommandList.Get(), rtv, std::array{0.01f, 0.01f, 0.01f, 1.0f});
		
		gfx::utils::ClearDepthBuffer(m_CommandList.Get(), dsv);

		m_CommandList->OMSetRenderTargets(1u, &rtv, FALSE, &dsv);

		// Remove after testing.

		auto vertexBufferView = m_VertexBuffer.GetBufferView();
		auto indexBufferView = m_IndexBuffer.GetBufferView();

		// --------------------------
		
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_CommandList->IASetVertexBuffers(0u, 1u, &vertexBufferView);
		m_CommandList->IASetIndexBuffer(&indexBufferView);
	
		// Update the MVP matrix
		dx::XMMATRIX mvpMatrix = dx::XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
		mvpMatrix = dx::XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);

		m_CommandList->SetGraphicsRoot32BitConstants(1u, sizeof(dx::XMMATRIX) / 4, &mvpMatrix, 0u);

		m_CommandList->DrawIndexedInstanced(36, 1u, 0u, 0u, 0u);
	
		gfx::utils::TransitionResource(m_CommandList.Get(), currentBackBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		ThrowIfFailed(m_CommandList->Close());

		std::array<ID3D12CommandList*, 1> commandLists
		{
			m_CommandList.Get()
		};

		m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());

		uint32_t syncInterval = m_VSync ? 1u : 0u;
		uint32_t presentFlags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0u;

		ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

		m_FrameFenceValues[m_CurrentBackBufferIndex] = Signal(m_CommandQueue.Get(), m_FenceValue);

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);

		m_FrameIndex++;
	}

	void SandBox::OnDestroy()
	{
		Flush(m_CommandQueue.Get(), m_FenceValue);

		::CloseHandle(m_FenceEvent);
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
			Flush(m_CommandQueue.Get(), m_FenceValue);

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

	void SandBox::InitRendererCore()
	{
		EnableDebugLayer();
		SelectAdapter();
		CreateDevice();

		m_CommandQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

		CheckTearingSupport();
		CreateSwapChain();

		m_RTVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, NUMBER_OF_FRAMES);
		
		m_DSVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1u);

		m_SRVDescriptor.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1u);

		CreateBackBufferRenderTargetViews();
		CreateDepthBuffer();

		for (wrl::ComPtr<ID3D12CommandAllocator>& commandAllocator : m_CommandAllocator)
		{
			commandAllocator = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		m_CommandList = CreateCommandList(m_CommandAllocator[m_CurrentBackBufferIndex].Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

		CreateFence();
		CreateEventHandle();

		m_Viewport =
		{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(m_Width),
			.Height = static_cast<float>(m_Height),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};
	}

	void SandBox::LoadContent()
	{
		// Reset command list and allocator for initial setup.
		ThrowIfFailed(m_CommandAllocator[m_CurrentBackBufferIndex].Get()->Reset());
		ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator[m_CurrentBackBufferIndex].Get(), nullptr));

		// Check for highest available version for root signature.
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData =
		{
			.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
		};

		if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData =
			{
				.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0
			};
		}

		// Create Root signature.
		std::array<CD3DX12_DESCRIPTOR_RANGE1, 1> descriptorRanges{};
		descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u, 1u, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		std::array<CD3DX12_ROOT_PARAMETER1, 2> rootParameters{};
		rootParameters[0].InitAsDescriptorTable(1u, descriptorRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsConstants(sizeof(dx::XMMATRIX) / 4, 0u, 0u, D3D12_SHADER_VISIBILITY_VERTEX);
		
		// Create static sampler.
		D3D12_STATIC_SAMPLER_DESC staticSamplerDesc
		{
			.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 0u,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
			.MinLOD = 0.0f,
			.MaxLOD = D3D12_FLOAT32_MAX,
			.ShaderRegister = 0u,
			.RegisterSpace = 1u,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
		};

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
		rootSignatureDesc.Init_1_1(static_cast<uint32_t>(rootParameters.size()), rootParameters.data(), 1u, &staticSamplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		wrl::ComPtr<ID3DBlob> rootSignatureBlob;
		wrl::ComPtr<ID3DBlob> errorBlob;

		ThrowIfFailed(::D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
		ThrowIfFailed(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

		// Create PSO and shaders.
		wrl::ComPtr<ID3DBlob> vertexShaderBlob;
		wrl::ComPtr<ID3DBlob> pixelShaderBlob;

		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/TestVS.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Shaders/TestPS.cso", &pixelShaderBlob));

		std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputElementDesc
		{{
			{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},

			{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = sizeof(dx::XMFLOAT3),
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		}};

		// Create PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = m_RootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get()),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState
			{
				.DepthEnable = TRUE,
				.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
				.StencilEnable = FALSE
			},
			.InputLayout =
			{
				.pInputElementDescs = inputElementDesc.data(),
				.NumElements = static_cast<UINT>(inputElementDesc.size()),
			},
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1u,
			.RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
			.DSVFormat = {DXGI_FORMAT_D32_FLOAT},
			.SampleDesc
			{
				.Count = 1u,
				.Quality = 0u
			},
			.NodeMask = 0u,
		};

		ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));

		// Creating vertex buffer for cube

		std::array<Vertex, 8> cubeVertices
		{
			Vertex{.position =  dx::XMFLOAT3(-1.0f, -1.0f, -1.0f),	.texCoord = dx::XMFLOAT2(0.0f, 1.0) }, // 0
			Vertex{.position =  dx::XMFLOAT3(-1.0f,  1.0f, -1.0f),	.texCoord = dx::XMFLOAT2(0.0f, 1.0f) }, // 1
			Vertex{.position =  dx::XMFLOAT3(1.0f,  1.0f, -1.0f),	.texCoord = dx::XMFLOAT2(1.0f, 1.0f) }, // 2
			Vertex{.position =  dx::XMFLOAT3(1.0f, -1.0f, -1.0f),	.texCoord = dx::XMFLOAT2(1.0f, 1.0f) }, // 3
			Vertex{.position =  dx::XMFLOAT3(-1.0f, -1.0f,  1.0f),	.texCoord = dx::XMFLOAT2(1.0f, 1.0f) }, // 4
			Vertex{.position =  dx::XMFLOAT3(-1.0f,  1.0f,  1.0f),	.texCoord = dx::XMFLOAT2(0.0f, 1.0f) }, // 5
			Vertex{.position =  dx::XMFLOAT3(1.0f,  1.0f,  1.0f),	.texCoord = dx::XMFLOAT2(1.0f, 1.0f) }, // 6
			Vertex{.position =  dx::XMFLOAT3(1.0f, -1.0f,  1.0f),	.texCoord = dx::XMFLOAT2(1.0f, 1.0f) }  // 7
		};

		m_VertexBuffer.Init<Vertex>(m_Device.Get(), m_CommandList.Get(), cubeVertices);

		// Create index buffer for cube

		std::array<uint32_t, 36> cubeIndices
		{
			0u, 1u, 2u, 0u, 2u, 3u,
			4u, 6u, 5u, 4u, 7u, 6u,
			4u, 5u, 1u, 4u, 1u, 0u,
			3u, 2u, 6u, 3u, 6u, 7u,
			1u, 5u, 6u, 1u, 6u, 2u,
			4u, 0u, 3u, 4u, 3u, 7u
		};

		m_IndexBuffer.Init(m_Device.Get(), m_CommandList.Get(), cubeIndices);

		// This heap is required to be non - null until the GPU finished operating on it.
		wrl::ComPtr<ID3D12Resource> textureUploadHeap;
		
		// Create texture
		int width{};
		int height{};
		int pixelChannels{};

		void* data = stbi_load("Assets/Textures/TestTexture.png", &width, &height, &pixelChannels, 0);

		D3D12_RESOURCE_DESC textureDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = static_cast<UINT>(width),
			.Height = static_cast<UINT>(height),
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0
			},
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		// Create intermediate resoruce to place texture in GPU accesible memory.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_Device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_Texture)));
		
		// Create GPU buffer for texture

		const uint64_t uploadBufferSize = GetRequiredIntermediateSize(m_Texture.Get(), 0u, 1u);

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		ThrowIfFailed(m_Device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap)));

		// Copy data from intermediate buffer to the upload heap.
		D3D12_SUBRESOURCE_DATA textureSubresourceData
		{
			.pData = data,
			.RowPitch = width * pixelChannels,
			.SlicePitch = width * pixelChannels * height
		};

		UpdateSubresources(m_CommandList.Get(), m_Texture.Get(), textureUploadHeap.Get(), 0u, 0u, 1u, &textureSubresourceData);
		
		stbi_image_free(data);

		// Transition resource from copy dest to Pixel SRV.
		gfx::utils::TransitionResource(m_CommandList.Get(), m_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Create SRV for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = textureDesc.Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MipLevels = 1
			}
		};

		m_Device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, m_SRVDescriptor.GetCPUDescriptorHandle());

		// Close command list and execute it (for the initial setup).
		ThrowIfFailed(m_CommandList->Close());
		std::array<ID3D12CommandList*, 1> commandLists
		{
			m_CommandList.Get()
		};

		m_CommandQueue->ExecuteCommandLists(static_cast<uint32_t>(commandLists.size()), commandLists.data());

		Flush(m_CommandQueue.Get(), m_FenceValue);
	}

	void SandBox::EnableDebugLayer()
	{
#ifdef _DEBUG
		ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&m_DebugInterface)));
		m_DebugInterface->EnableDebugLayer();
		m_DebugInterface->SetEnableGPUBasedValidation(TRUE);
		m_DebugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
#endif
	}

	void SandBox::SelectAdapter()
	{
		wrl::ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;

#ifdef _DEBUG
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		wrl::ComPtr<IDXGIAdapter4> adapter4;
		wrl::ComPtr<IDXGIAdapter1> adapter1;

		// Prefer adapter with highest available dedicated video memory.
		SIZE_T maximumDedicatedVideoMemory{};
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc{};
			adapter1->GetDesc1(&adapterDesc);

			if ((!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) &&
				(SUCCEEDED(::D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr))) &&
				(adapterDesc.DedicatedVideoMemory > maximumDedicatedVideoMemory))
			{
				maximumDedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
				ThrowIfFailed(adapter1.As(&adapter4));
			}
		}

		m_Adapter = adapter4;
	}

	void SandBox::CreateDevice()
	{
		ThrowIfFailed(::D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));

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

		D3D12_INFO_QUEUE_FILTER infoQueueFilter
		{
			.DenyList
			{
				.NumSeverities = static_cast<UINT>(ignoreMessageSeverities.size()),
				.pSeverityList = ignoreMessageSeverities.data()
			},
		};

		ThrowIfFailed(infoQueue->PushStorageFilter(&infoQueueFilter));
		
#endif
	}

	wrl::ComPtr<ID3D12CommandQueue> SandBox::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE)
	{
		wrl::ComPtr<ID3D12CommandQueue> commandQueue;

		D3D12_COMMAND_QUEUE_DESC commandQueueDesc
		{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0,
		};

		ThrowIfFailed(m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));

		return commandQueue;
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
		wrl::ComPtr<IDXGIFactory4> dxgiFactory;
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
		ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_CommandQueue.Get(), Application::GetWindowHandle(), &swapChainDesc, nullptr, nullptr, &swapChain1));

		// Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
		ThrowIfFailed(dxgiFactory->MakeWindowAssociation(Application::GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));
		
		ThrowIfFailed(swapChain1.As(&m_SwapChain));

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
	}

	wrl::ComPtr<ID3D12DescriptorHeap>  SandBox::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t descriptorCount)
	{
		wrl::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc
		{
			.Type = descriptorHeapType,
			.NumDescriptors = descriptorCount,
			.Flags = heapFlags,
			.NodeMask = 0
		};

		ThrowIfFailed(m_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}

	void SandBox::CreateBackBufferRenderTargetViews()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptor.GetCPUDescriptorHandle());

		for (int i = 0; i < NUMBER_OF_FRAMES; ++i)
		{
			wrl::ComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

			m_BackBuffers[i] = backBuffer;

			rtvHandle.Offset(m_RTVDescriptor.GetDescriptorSize());
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

		m_Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DSVDescriptor.GetCPUDescriptorHandle());
	}

	wrl::ComPtr<ID3D12CommandAllocator> SandBox:: CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
	{
		wrl::ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(m_Device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));

		return commandAllocator;
	}

	wrl::ComPtr<ID3D12GraphicsCommandList> SandBox::CreateCommandList(ID3D12CommandAllocator* commandAllocator, D3D12_COMMAND_LIST_TYPE commandListType)
	{
		wrl::ComPtr<ID3D12GraphicsCommandList> graphicsCommandList;
		ThrowIfFailed(m_Device->CreateCommandList(0, commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&graphicsCommandList)));

		// Closing the command list as they are created in recording state adn for first iteration of render loop it is needed to be reset.
		ThrowIfFailed(graphicsCommandList->Close());

		return graphicsCommandList;
	}

	void SandBox::CreateFence()
	{
		ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
	}

	void SandBox::CreateEventHandle()
	{
		m_FenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	uint64_t SandBox::Signal(ID3D12CommandQueue* commandQueue, uint64_t& fenceValue)
	{
		uint64_t fenceValueForSignal = ++fenceValue;

		ThrowIfFailed(commandQueue->Signal(m_Fence.Get(), fenceValueForSignal));
		
		return fenceValueForSignal;
	}

	void SandBox::WaitForFenceValue(uint64_t fenceValue, std::chrono::milliseconds duration)
	{
		if (m_Fence->GetCompletedValue() < fenceValue)
		{
			ThrowIfFailed(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
			::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(duration.count()));
		}
	}

	void SandBox::Flush(ID3D12CommandQueue* commandQueue, uint64_t& fenceValue)
	{
		uint64_t fenceValueForSignal = Signal(commandQueue, fenceValue);
		WaitForFenceValue(fenceValueForSignal);
	}
}
