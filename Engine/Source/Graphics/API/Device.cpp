

#include "Device.hpp"
#include "Core/Application.hpp"

#include "Common/ConstantBuffers.hlsli"

#include "stb_image.h"

#include "Editor/Log.hpp"

// For setting the Agility SDK paramters.
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 602u;
}
extern "C"
{
    __declspec(dllexport) extern const char *D3D12SDKPath = ".\\D3D12\\";
}

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace Microsoft::WRL;

namespace helios::gfx
{
	Device::Device()
	{
		InitDeviceResources();
		InitSwapChainResources();

		mIsInitialized = true;

		helios::editor::LogMessage(L"Initialized Device", editor::LogMessageTypes::Info);
	}

	Device::~Device()
	{
		mGraphicsCommandQueue->FlushQueue();
		mComputeCommandQueue->FlushQueue();

		RenderTarget::DestroyResources();
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

		// Setup DRED.
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&mDredSettings)));
		mDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);	
		mDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
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
		// note(rtarun9) : srvCbvUav descriptor count will be very high, because of mip maps.
		mRtvDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 50u, L"RTV Descriptor");
		mDsvDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 15u, L"DSV Descriptor");
		mSrvCbvUavDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 8192u, L"SRV_CBV_UAV Descriptor");
		mSamplerDescriptor = std::make_unique<Descriptor>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1000u, L"Sampler Descriptor");

		// Create bindless root signature.
		PipelineState::CreateBindlessRootSignature(mDevice.Get(), (L"Shaders/BindlessRS.cso"));

		// Create render target resources.
		RenderTarget::CreateRenderTargetResources(this);
		
		// Create mip map generator (required bindless RS to be created first).
		mMipMapGenerator = std::make_unique<MipMapGenerator>(this);
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
			.Width = core::Application::GetClientDimensions().x,
			.Height = core::Application::GetClientDimensions().y,
			.Format = Device::SWAPCHAIN_FORMAT,
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
		ThrowIfFailed(mFactory->CreateSwapChainForHwnd(mGraphicsCommandQueue->GetCommandQueue().Get(), core::Application::GetWindowHandle(), &swapChainDesc, nullptr, nullptr, &swapChain1));

		// Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
		ThrowIfFailed(mFactory->MakeWindowAssociation(core::Application::GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain1.As(&mSwapChain));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
		
		CreateBackBufferRTVs();
	}

	void Device::CreateBackBufferRTVs()
	{
		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
		
		// The first NUMBER_OF_FRAMES's rtv descriptor handles are reserved for the swapchain buffers.
		gfx::DescriptorHandle rtvHandle = mRtvDescriptor->GetDescriptorHandleFromStart();
		
		// Create Backbuffer render target views.
		for (int i : std::views::iota(0u, NUMBER_OF_FRAMES))
		{

			wrl::ComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle.cpuDescriptorHandle);

			mBackBuffers[i].backBufferResource = backBuffer;
			mBackBuffers[i].backBufferResource->SetName(L"SwapChain BackBuffer");

			mBackBuffers[i].backBufferDescriptorHandle = rtvHandle;

			mRtvDescriptor->Offset(rtvHandle);
		}

		if (!mIsInitialized)
		{
			mRtvDescriptor->OffsetCurrentHandle(NUMBER_OF_FRAMES);
		}
	}

	void Device::ResizeBuffers()
	{
		mGraphicsCommandQueue->FlushQueue();
		mCopyCommandQueue->FlushQueue();
		mComputeCommandQueue->FlushQueue();

		// Resize the swap chain's back buffer.
		for (int i  : std::views::iota(0u, NUMBER_OF_FRAMES))
		{
			mBackBuffers[i].backBufferResource.Reset();
			mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(mSwapChain->ResizeBuffers(NUMBER_OF_FRAMES, core::Application::GetClientDimensions().x, core::Application::GetClientDimensions().y, DXGI_FORMAT_UNKNOWN, swapChainDesc.Flags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		CreateBackBufferRTVs();
	}

	void Device::BeginFrame()
	{
	}

	void Device::EndFrame()
	{
	}

	void Device::ExecuteContext(std::span<std::unique_ptr<GraphicsContext>> graphicsContext)
	{
		std::vector<ID3D12GraphicsCommandList1*> commandLists{};
		for (auto& list : graphicsContext)
		{
			commandLists.push_back(list->GetCommandList());
		}

		mFrameFenceValues[mCurrentBackBufferIndex] = mGraphicsCommandQueue->ExecuteCommandLists(commandLists);
	}

	void Device::ExecuteContext(std::span<std::unique_ptr<ComputeContext>> computeContext)
	{
		std::vector<ID3D12GraphicsCommandList1*> commandLists{};
		for (auto& list : computeContext)
		{
			commandLists.push_back(list->GetCommandList());
		}

		// Execute commands recorded into the graphics context.
		mFrameFenceValues[mCurrentBackBufferIndex] = mComputeCommandQueue->ExecuteCommandLists(commandLists);
	}

	// Forward argument in a 'span compatible format' other overload.
	void Device::ExecuteContext(std::unique_ptr<GraphicsContext> graphicsContext)
	{
		std::array<std::unique_ptr<GraphicsContext>, 1u> context{ std::move(graphicsContext) };
		ExecuteContext(context);
	}

	void Device::ExecuteContext(std::unique_ptr<ComputeContext> computeContext)
	{
		std::array<std::unique_ptr<ComputeContext>, 1u> context{ std::move(computeContext) };
		ExecuteContext(context);
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
		uint32_t srvIndex = mSrvCbvUavDescriptor->GetCurrentDescriptorIndex();
		mDevice->CreateShaderResourceView(resource, &srvCreationDesc.srvDesc, mSrvCbvUavDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);
		
		mSrvCbvUavDescriptor->OffsetCurrentHandle();

		return srvIndex;
	}

	uint32_t Device::CreateRtv(const RtvCreationDesc& rtvCreationDesc, ID3D12Resource* resource) const
	{
		uint32_t rtvIndex = mRtvDescriptor->GetCurrentDescriptorIndex();
		mDevice->CreateRenderTargetView(resource, nullptr,  mRtvDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);
		
		mRtvDescriptor->OffsetCurrentHandle();

		return rtvIndex;
	}

	uint32_t Device::CreateDsv(const DsvCreationDesc& dsvCreationDesc, ID3D12Resource* resource) const
	{
		uint32_t dsvIndex = mDsvDescriptor->GetCurrentDescriptorIndex();
		mDevice->CreateDepthStencilView(resource, &dsvCreationDesc.dsvDesc, mDsvDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);

		mDsvDescriptor->OffsetCurrentHandle();

		return dsvIndex;
	}

	uint32_t Device::CreateUav(const UavCreationDesc& uavCreationDesc, ID3D12Resource* resource) const
	{
		uint32_t uavIndex = mSrvCbvUavDescriptor->GetCurrentDescriptorIndex();
		mDevice->CreateUnorderedAccessView(resource, nullptr, &uavCreationDesc.uavDesc, mSrvCbvUavDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);

		mSrvCbvUavDescriptor->OffsetCurrentHandle();

		return uavIndex;
	}

	uint32_t Device::CreateCbv(const CbvCreationDesc& cbvCreationDesc) const
	{
		uint32_t cbvIndex = mSrvCbvUavDescriptor->GetCurrentDescriptorIndex();

		mDevice->CreateConstantBufferView(&cbvCreationDesc.cbvDesc, mSrvCbvUavDescriptor->GetCurrentDescriptorHandle().cpuDescriptorHandle);

		mSrvCbvUavDescriptor->OffsetCurrentHandle();

		return cbvIndex;
	}

	uint32_t Device::CreateSampler(const SamplerCreationDesc& samplerCreationDesc) const
	{
		uint32_t samplerIndex = mSamplerDescriptor->GetCurrentDescriptorIndex();
		gfx::DescriptorHandle samplerDescriptorHandle = mSamplerDescriptor->GetCurrentDescriptorHandle();

		mDevice->CreateSampler(&samplerCreationDesc.samplerDesc, samplerDescriptorHandle.cpuDescriptorHandle);

		return samplerIndex;
	}
	
	Texture Device::CreateTexture(TextureCreationDesc& textureCreationDesc, const unsigned char* data) const
	{
		Texture texture{};

	    textureCreationDesc.path = utility::ResourceManager::GetAssetPath(textureCreationDesc.path);

		int componentCount{ 4 }, width{}, height{};

		// For use only by HDR textures (mostly for Cube Map equirectangular textures).
		float* hdrTextureData{ nullptr };

		if (textureCreationDesc.usage == TextureUsage::TextureFromData)
		{
			if (!data)
			{
				throw std::runtime_error("Texture usage : TextureFromData but no data provided.");
			}
			
			width = textureCreationDesc.dimensions.x;
			height = textureCreationDesc.dimensions.y;
		}

		if (textureCreationDesc.usage == TextureUsage::HDRTextureFromPath)
		{
            hdrTextureData = stbi_loadf(WstringToString( textureCreationDesc.path).c_str(), &width, &height, nullptr, componentCount);
			if (!hdrTextureData)
			{
				ErrorMessage(L"Failed to load texture from path : " + textureCreationDesc.path);
			}

			textureCreationDesc.dimensions =
			{
				.x = static_cast<uint32_t>(width),
				.y = static_cast<uint32_t>(height)
			};
		}

		if (textureCreationDesc.usage == TextureUsage::TextureFromPath)
		{
            data = stbi_load(WstringToString( (textureCreationDesc.path)).c_str(), &width, &height, nullptr, componentCount);
			if (!data)
			{
				ErrorMessage(L"Failed to load texture from path : " + textureCreationDesc.path);
			}

			textureCreationDesc.dimensions =
			{
				.x = static_cast<uint32_t>(width),
				.y = static_cast<uint32_t>(height)
			};
		}

		std::lock_guard<std::recursive_mutex> resourceLockGuard(sResourceMutex);

		texture.allocation = mMemoryAllocator->CreateTextureResourceAllocation(textureCreationDesc);

		texture.dimensions = textureCreationDesc.dimensions;

		uint32_t mipLevels = textureCreationDesc.mipLevels;

		// Needed here as we can pass formats specific for depth stencil texture (DXGI_FORMAT_D32_FLOAT) or formats used by textures / render targets (DXGI_FORMAT_R32G32B32A32_FLOAT).
		DXGI_FORMAT format{textureCreationDesc.format};
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
		SrvCreationDesc srvCreationDesc{};

		if (textureCreationDesc.depthOrArraySize == 1u)
		{
			srvCreationDesc = 
			{
				.srvDesc
				{

					.Format = format,
					.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Texture2D
					{
						.MostDetailedMip = 0u,
						.MipLevels = mipLevels
					}
				}
			};
		}
		else if (textureCreationDesc.depthOrArraySize == 6u)
		{
			srvCreationDesc =
			{
				.srvDesc
				{
					.Format = format,
					.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.TextureCube
					{
						.MostDetailedMip = 0u,
						.MipLevels = mipLevels
					}
				}
			};
		}

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
		
		// Create RTV (if applicable).
		if (textureCreationDesc.usage == TextureUsage::RenderTarget)
		{
			RtvCreationDesc rtvCreationDesc
			{
				.rtvDesc
				{
					.Format = format,
					.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = 0u,
						.PlaneSlice = 0u
					},
				}
			};

			texture.rtvIndex = CreateRtv(rtvCreationDesc, texture.allocation->resource.Get());
		}

		// Create UAV (if applicable).
		if (textureCreationDesc.usage == TextureUsage::CubeMap)
		{
			UavCreationDesc uavCreationDesc
			{
				.uavDesc
				{
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
					.Texture2DArray
					{
						.MipSlice = 0u,
						.FirstArraySlice = 0u,
						.ArraySize = textureCreationDesc.depthOrArraySize
					}
				}
			};
			
			texture.uavIndex = CreateUav(uavCreationDesc, texture.allocation->resource.Get());
		}

		// If texture created from file, load data (etc using stb_image) into a upload buffer and copy subresource data accordingly.
		if (textureCreationDesc.usage == TextureUsage::TextureFromPath || textureCreationDesc.usage == TextureUsage::TextureFromData || textureCreationDesc.usage == TextureUsage::HDRTextureFromPath)
		{		
			// Create upload buffer.
			BufferCreationDesc uploadBufferCreationDesc
			{
				.usage = BufferUsage::UploadBuffer,
				.name = L"Upload buffer - " + textureCreationDesc.name,
			};

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.allocation->resource.Get(), 0, 1);
			
			ResourceCreationDesc resourceCreationDesc = ResourceCreationDesc::CreateBufferResourceCreationDesc(uploadBufferSize);

			std::unique_ptr<Allocation> uploadAllocation = mMemoryAllocator->CreateBufferResourceAllocation(uploadBufferCreationDesc, resourceCreationDesc);

			// Specify data to copy.
			D3D12_SUBRESOURCE_DATA textureSubresourceData{};

			if (textureCreationDesc.usage == TextureUsage::HDRTextureFromPath)
			{
				textureSubresourceData = 
				{
					.pData = hdrTextureData,
					.RowPitch = width * componentCount * 4,
					.SlicePitch = width * height * componentCount * 4
				};
			}
			else
			{
				textureSubresourceData =
				{
					.pData = data,
					.RowPitch = width * componentCount,
					.SlicePitch = width * height * componentCount
				};
			}
			
			// Get a copy command and list and execute UpdateSubresources functions on the command queue.
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> copyCommandList = mCopyCommandQueue->GetCommandList();

			UpdateSubresources(copyCommandList.Get(), texture.allocation->resource.Get(), uploadAllocation->resource.Get(), 0u, 0u, 1u, &textureSubresourceData);

			mCopyCommandQueue->ExecuteAndFlush(copyCommandList.Get());

			uploadAllocation->Reset();
		}
		
		// Now that data is copied / set into GPU memory, freeing it.
		if (data)
		{
			stbi_image_free((void*)data);
		}

		if (hdrTextureData)
		{
			stbi_image_free(hdrTextureData);
		}

		// Generate mip maps.
		texture.textureName = textureCreationDesc.name;
		mMipMapGenerator->GenerateMips(&texture);

		editor::LogMessage(L"Created texture : " + texture.textureName, editor::LogMessageTypes::Info);

		return texture;
	}

	RenderTarget Device::CreateRenderTarget(TextureCreationDesc& textureCreationDesc) const
	{
		RenderTarget renderTarget{};

		renderTarget.renderTexture = std::make_unique<gfx::Texture>(CreateTexture(textureCreationDesc));
		renderTarget.renderTargetName = textureCreationDesc.name;

	    editor::LogMessage(L"Created render target : " + renderTarget.renderTargetName, editor::LogMessageTypes::Info);

		return renderTarget;
	}

	void Device::ResizeRenderTarget(RenderTarget* renderTarget)
	{
		D3D12_RESOURCE_DESC resourceDesc = renderTarget->renderTexture->GetResource()->GetDesc();
		resourceDesc.Width = core::Application::GetClientDimensions().x;
		resourceDesc.Height = core::Application::GetClientDimensions().y;

		// Recreate RTV.
		TextureCreationDesc textureCreationDesc
		{
			.usage = gfx::TextureUsage::RenderTarget,
			.dimensions = core::Application::GetClientDimensions(),
			.format = resourceDesc.Format,
			.name = renderTarget->renderTargetName
		};

		renderTarget->renderTexture->allocation->Reset();

		// Recreate allocation.
		renderTarget->renderTexture->allocation = mMemoryAllocator->CreateTextureResourceAllocation(textureCreationDesc);

		DescriptorHandle rtvHandle = mRtvDescriptor->GetDescriptorHandleFromIndex(renderTarget->renderTexture->rtvIndex);
		mDevice->CreateRenderTargetView(renderTarget->renderTexture->GetResource(), nullptr, rtvHandle.cpuDescriptorHandle);

		// ReCreate SRV.
		SrvCreationDesc srvCreationDesc
		{
			.srvDesc
			{
				.Format = resourceDesc.Format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D
				{
					.MostDetailedMip = 0u,
					.MipLevels = resourceDesc.MipLevels
				}
			}
		};
		
		DescriptorHandle srvHandle = mSrvCbvUavDescriptor->GetDescriptorHandleFromIndex(renderTarget->renderTexture->srvIndex);
		mDevice->CreateShaderResourceView(renderTarget->renderTexture->GetResource(), &srvCreationDesc.srvDesc, srvHandle.cpuDescriptorHandle);
	}

	PipelineState Device::CreatePipelineState(const GraphicsPipelineStateCreationDesc& graphicsPipelineStateCreationDesc) const
	{
		PipelineState pipelineState(mDevice.Get(), graphicsPipelineStateCreationDesc);

        editor::LogMessage(L"Created compute pipeline state : " + graphicsPipelineStateCreationDesc.pipelineName, editor::LogMessageTypes::Info);

		return pipelineState;
	}

	PipelineState Device::CreatePipelineState(const ComputePipelineStateCreationDesc& computePipelineStateCreationDesc) const
	{
		PipelineState pipelineState(mDevice.Get(), computePipelineStateCreationDesc);

	    editor::LogMessage(L"Created compute pipeline state : " + computePipelineStateCreationDesc.pipelineName, editor::LogMessageTypes::Info);

		return pipelineState;
	}
}

