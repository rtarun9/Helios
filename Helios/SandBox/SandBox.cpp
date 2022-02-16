#include "Pch.hpp"

#include "SandBox.hpp"

#include "Core/Application.hpp"

namespace helios
{
	SandBox::SandBox(Config& config)
		: Engine(config)
	{
	}

	void SandBox::OnInit()
	{
		InitRendererCore();
	}

	void SandBox::OnUpdate()
	{

	}

	void SandBox::OnRender()
	{
		wrl::ComPtr<ID3D12CommandAllocator> commandAllocator = m_CommandAllocator[m_CurrentBackBufferIndex];
		wrl::ComPtr<ID3D12Resource> currentBackBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

		commandAllocator->Reset();

		ThrowIfFailed(m_CommandList->Reset(commandAllocator.Get(), nullptr));

		// Clear RTV
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

			m_CommandList->ResourceBarrier(1, &barrier);

			float clearColor[] = { 0.8f, 0.1f, 0.7f, 1.0f };
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);

			m_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		}

		// Present
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			
			m_CommandList->ResourceBarrier(1, &barrier);

			ThrowIfFailed(m_CommandList->Close());

			ID3D12CommandList* const commandLists[] =
			{
				m_CommandList.Get()
			};

			m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

			uint32_t syncInterval = m_VSync ? 1u : 0u;
			uint32_t presentFlags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0u;

			ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

			m_FrameFenceValues[m_CurrentBackBufferIndex] = Signal(m_CommandQueue.Get(), m_FenceValue);

			m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

			WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
		}
	}

	void SandBox::OnDestroy()
	{
		Flush(m_CommandQueue.Get(), m_FenceValue);
	}

	void SandBox::OnKeyDown(uint8_t keycode)
	{

	}

	void SandBox::OnKeyUp(uint8_t keycode)
	{

	}

	void SandBox::InitRendererCore()
	{

		EnableDebugLayer();
		SelectAdapter();
		CreateDevice();

		m_CommandQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

		CheckTearingSupport();
		CreateSwapChain();

		m_RTVDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUMBER_OF_FRAMES);
		
		CreateBackBufferRenderTargetViews();

		for (wrl::ComPtr<ID3D12CommandAllocator>& commandAllocator : m_CommandAllocator)
		{
			commandAllocator = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		m_CommandList = CreateCommandList(m_CommandAllocator[m_CurrentBackBufferIndex].Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

		CreateFence();
		CreateEventHandle();
	}

	void SandBox::EnableDebugLayer()
	{
#ifdef _DEBUG
		wrl::ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
#endif
	}

	void SandBox::SelectAdapter()
	{
		wrl::ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;

#ifdef _DEBUG
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		wrl::ComPtr<IDXGIAdapter4> adapter4;
		wrl::ComPtr<IDXGIAdapter1> adapter1;

		// Prefer adapter with highest available dedicated video memory.
		SIZE_T maximumDedicatedVideoMemory{};
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc{};
			adapter1->GetDesc1(&adapterDesc);

			if ((!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) &&
				(SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr))) &&
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
		ThrowIfFailed(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));

		// Set break points on certain severity levels in debug mode.
#ifdef _DEBUG
		wrl::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(m_Device.As(&infoQueue)))
		{
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
		}
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

		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

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

	wrl::ComPtr<ID3D12DescriptorHeap> SandBox::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t descriptorCount)
	{
		wrl::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc
		{
			.Type = descriptorHeapType,
			.NumDescriptors = descriptorCount,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		ThrowIfFailed(m_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}

	void SandBox::CreateBackBufferRenderTargetViews()
	{
		m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < NUMBER_OF_FRAMES; ++i)
		{
			wrl::ComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

			m_BackBuffers[i] = backBuffer;

			rtvHandle.Offset(m_RTVDescriptorSize);
		}
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
