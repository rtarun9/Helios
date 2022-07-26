#include "Pch.hpp"

#include "Device.hpp"
#include "Core/Application.hpp"

namespace helios::gfx
{
	Device::Device()
	{
		InitDeviceResources();
		InitSwapChainResources();
	}

	Device::~Device()
	{
		mGraphicsCommandQueue->FlushQueue();
		mComputeCommandQueue->FlushQueue();
	}

	void Device::InitDeviceResources()
	{
		// Enable the debug layer, so that all errors generated while creating DX12 objects is caught.
#ifdef _DEBUG
		ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugInterface)));
		mDebugInterface->EnableDebugLayer();
		mDebugInterface->SetEnableGPUBasedValidation(TRUE);
		mDebugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
		mDebugInterface->SetEnableAutoName(TRUE);
#endif

		// Create DXGI Factory.
		UINT dxgiFactoryFlags{ 0 };
#ifdef _DEBUG
		dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(::CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

		// Select the adapter (in this case GPU with best performance).
		// Index 0 will be the GPU with highest preference.
		ThrowIfFailed(mFactory->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter)));

		if (!mAdapter)
		{
			ErrorMessage(L"Could not get a D3D12 compatible adapter.");
		}

#ifdef _DEBUG
		// The adapter descriptor will be displayed as message (check infoQueue below).
		DXGI_ADAPTER_DESC adapterDesc{};
		ThrowIfFailed(mAdapter->GetDesc(&adapterDesc));
		std::string adapterInfo{ "\nAdapter Description : " + WstringToString(adapterDesc.Description) + ".\n" };
#endif

		// Create D3D12 device.
		ThrowIfFailed(::D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));
		mDevice->SetName(L"D3D12 Device");

		// Set break points on certain severity levels in debug mode, and enable debug messages in Debug Mode.
#ifdef _DEBUG
		wrl::ComPtr<ID3D12InfoQueue> infoQueue;
		ThrowIfFailed(mDevice.As(&infoQueue));

		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		infoQueue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_MESSAGE, adapterInfo.c_str());

		// Configure queue filter to ignore info message severity.
		std::array<D3D12_MESSAGE_SEVERITY, 1> ignoreMessageSeverities
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Configure queue filter to ignore individual messages using thier ID.
		std::array<D3D12_MESSAGE_ID, 2> ignoreMessageIDs
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
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

		// Get the debug device. It represents a graphics device for debugging, while debug interface controls debug settings and validates pipeline state.
		ThrowIfFailed(mDevice->QueryInterface(IID_PPV_ARGS(&mDebugDevice)));
#endif

		// Create memory allocator.
		mMemoryAllocator = std::make_unique<MemoryAllocator>(mDevice.Get(), mAdapter.Get());

		// Create the command queue's.
		mGraphicsCommandQueue = std::make_unique<CommandQueue>(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Graphics Command Queue");
		mComputeCommandQueue = std::make_unique<CommandQueue>(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Compute Command Queue");
		mCopyCommandQueue = std::make_unique<CommandQueue>(mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COPY, L"Copy Command Queue");
		
		// Create the descriptor heaps.
		mRtvDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"RTV Descriptor");
		mDsvDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"DSV Descriptor");
		mSrvCbvUavDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1020u, L"SRV_CBV_UAV Descriptor");

		// Create bindless root signature.
		PipelineState::CreateBindlessRootSignature(mDevice.Get(), L"Shaders/BindlessRS.cso");
	}

	void Device::InitSwapChainResources()
	{
		// Check if tearing is supported (needed to know if tearing should be done when vsync is off).
		BOOL allowTearing = FALSE;
		if (FAILED(mFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			allowTearing = FALSE;
		}

		mTearingSupported = allowTearing;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc
		{
			.Width = Application::GetClientDimensions().x,
			.Height = Application::GetClientDimensions().y,
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
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = mTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
		};

		wrl::ComPtr<IDXGISwapChain1> swapChain1;
		ThrowIfFailed(mFactory->CreateSwapChainForHwnd(mGraphicsCommandQueue->GetCommandQueue().Get(), Application::GetWindowHandle(), &swapChainDesc, nullptr, nullptr, &swapChain1));

		// Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
		ThrowIfFailed(mFactory->MakeWindowAssociation(Application::GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain1.As(&mSwapChain));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
		
		CreateBackBufferRTVs();
	}

	void Device::CreateBackBufferRTVs()
	{
		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
		
		// Create Backbuffer render target views.
		for (int i : std::views::iota(0u, NUMBER_OF_FRAMES))
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle;

			wrl::ComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

			mBackBuffers[i].backBufferResource = backBuffer;
			mBackBuffers[i].backBufferResource->SetName(L"SwapChain BackBuffer");

			mBackBuffers[i].backBufferDescriptorHandle = mRtvDescriptor->GetCurrentDescriptorHandle();

			mRtvDescriptor->OffsetCurrentHandle();
		}
	}

	void Device::ResizeBuffers()
	{
		mGraphicsCommandQueue->FlushQueue();

		// Resize the swap chain's back buffer.
		for (int i = 0; i < NUMBER_OF_FRAMES; i++)
		{
			mBackBuffers[i].backBufferResource.Reset();
			mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(mSwapChain->ResizeBuffers(NUMBER_OF_FRAMES, Application::GetClientDimensions().x, Application::GetClientDimensions().y, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		CreateBackBufferRTVs();
	}

	void Device::BeginFrame()
	{
	}

	void Device::EndFrame()
	{
	}

	void Device::ExecuteContext(std::unique_ptr<GraphicsContext> graphicsContext)
	{
		// Execute commands recorded into the graphics context.
		mFrameFenceValues[mCurrentBackBufferIndex] = mGraphicsCommandQueue->ExecuteCommandList(graphicsContext->GetCommandList());
	}

	void Device::Present()
	{
		uint32_t syncInterval = mVSync ? 1u : 0u;
		uint32_t presentFlags = mTearingSupported && !mVSync ? DXGI_PRESENT_ALLOW_TEARING : 0u;

		ThrowIfFailed(mSwapChain->Present(syncInterval, presentFlags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		mGraphicsCommandQueue->WaitForFenceValue(mFrameFenceValues[mCurrentBackBufferIndex]);
	}

	uint32_t Device::CreateSrv(const SrvCreationDesc& srvCreationDesc, ID3D12Resource* resource) const
	{
		uint32_t srbCbvUavIndex = mSrvCbvUavDescriptor->GetCurrentDescriptorIndex();
		mDevice->CreateShaderResourceView(resource, &srvCreationDesc.srvDesc, mSrvCbvUavDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);
		
		mSrvCbvUavDescriptor->OffsetCurrentHandle();

		return srbCbvUavIndex;
	}

	uint32_t Device::CreateRtv(const RtvCreationDesc& rtvCreationDesc, ID3D12Resource* resource) const
	{
		mDevice->CreateRenderTargetView(resource, nullptr,  mSrvCbvUavDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);
		mRtvDescriptor->OffsetCurrentHandle();

		return mRtvDescriptor->GetCurrentDescriptorIndex();
	}

	uint32_t Device::CreateDsv(const DsvCreationDesc& dsvCreationDesc, ID3D12Resource* resource) const
	{
		uint32_t dsvIndex = mDsvDescriptor->GetCurrentDescriptorIndex();
		mDevice->CreateDepthStencilView(resource, &dsvCreationDesc.dsvDesc, mDsvDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);

		mDsvDescriptor->OffsetCurrentHandle();

		return dsvIndex;
	}

	uint32_t Device::CreateCbv(const CbvCreationDesc& cbvCreationDesc) const
	{
		uint32_t cbvIndex = mSrvCbvUavDescriptor->GetCurrentDescriptorIndex();

		mDevice->CreateConstantBufferView(&cbvCreationDesc.cbvDesc, mSrvCbvUavDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);

		mSrvCbvUavDescriptor->OffsetCurrentHandle();

		return cbvIndex;
	}

	Texture Device::CreateTexture(const TextureCreationDesc& textureCreationDesc)
	{
		Texture texture{};

		texture.allocation = mMemoryAllocator->CreateTextureResourceAllocation(textureCreationDesc);

		// Needed here as we can pass formats specific for depth stencil texture (DXGI_FORMAT_D32_FLOAT) or formats used by textures / render targets (DXGI_FORMAT_R32G32B32A32_FLOAT).
		DXGI_FORMAT format{};
		DXGI_FORMAT dsFormat{};

		switch (textureCreationDesc.format)
		{
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_TYPELESS:
			{
				dsFormat = DXGI_FORMAT_D32_FLOAT;
				format = DXGI_FORMAT_R32_FLOAT;
			}break;

			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			{
				throw std::runtime_error("Currently, the renderer does not support depth format of the type D24_S8_UINT. Please use one of the X32 types.");
			}break;
		}

		// Create SRV.
		SrvCreationDesc srvCreationDesc
		{
			.srvDesc
			{
				.Format = format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D
				{
					.MostDetailedMip = 0u,
					.MipLevels = 1u
				}
			}
		};

		texture.srvIndex = CreateSrv(srvCreationDesc, texture.allocation->resource.Get());

		// Create DSV (if applicable).
		if (textureCreationDesc.usage == TextureUsage::DepthStencil)
		{
			DsvCreationDesc dsvCreationDesc
			{
				.dsvDesc
				{
					.Format = dsFormat,
					.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
					.Flags = D3D12_DSV_FLAG_NONE,
					.Texture2D
					{
						.MipSlice = 0u
					},
				}
			};

			texture.dsvIndex = CreateDsv(dsvCreationDesc, texture.allocation->resource.Get());
		}

		return texture;
	}

	PipelineState Device::CreatePipelineState(const GraphicsPipelineStateCreationDesc& graphicsPipelineStateCreationDesc) const
	{
		PipelineState pipelineState(mDevice.Get(), graphicsPipelineStateCreationDesc);

		return pipelineState;
	}
}

