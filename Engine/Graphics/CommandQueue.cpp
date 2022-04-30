#include "Pch.hpp"

#include "CommandQueue.hpp"

namespace helios::gfx
{
	void CommandQueue::Init(ID3D12Device* const device, D3D12_COMMAND_LIST_TYPE commandListType, std::wstring_view commandQueueName)
	{
		m_Device = device;
		m_CommandListType = commandListType;

		// Create the command queue based on list type.
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc
		{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};

		ThrowIfFailed(m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue)));
		m_CommandQueue->SetName(commandQueueName.data());

		// Create command queue sync objects.
		ThrowIfFailed(m_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
		std::wstring fenceName = commandQueueName.data() + std::wstring(L" Fence");
		m_Fence->SetName(fenceName.data());

		m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		
		if (!m_FenceEvent)
		{
			ErrorMessage(L"Failed to create fence event");
		}
	}
	
	wrl::ComPtr<ID3D12GraphicsCommandList> CommandQueue::GetCommandList()
	{
		// Check if there is command allocator (that allocates the list) is not in flight -> if so it can be reused.
		// Other wise a new command allocator has to be created.

		wrl::ComPtr<ID3D12GraphicsCommandList> commandList;
		wrl::ComPtr<ID3D12CommandAllocator> commandAllocator;

		if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
		{
			commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
			m_CommandAllocatorQueue.pop();

			ThrowIfFailed(commandAllocator->Reset());
		}
		else
		{
			commandAllocator = CreateCommandAllocator();
		}

		// Check if there are any command list in the queue, hence create a new one.
		if (!m_CommandListQueue.empty())
		{
			commandList = m_CommandListQueue.front();
			m_CommandListQueue.pop();

			ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
		}
		else
		{
			commandList = CreateCommandList(commandAllocator.Get());
		}

		// Create association between the command list and allocator so we can find out which command allocater the list was
		// allocated from.

		ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

		return commandList;
	}

	// Returns the fence value to wait for to notify when command list has finished execution.
	uint64_t CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* const commandList)
	{
		commandList->Close();

		ID3D12CommandAllocator* commandAllocator{ nullptr };
		UINT dataSize = sizeof(ID3D12CommandAllocator);

		ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

		std::array<ID3D12CommandList* const, 1> commandLists
		{
			commandList
		};

		m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());
		uint64_t fenceValue = Signal();

		m_CommandAllocatorQueue.emplace(CommandAllocator{ .fenceValue = fenceValue, .commandAllocator = commandAllocator });
		m_CommandListQueue.emplace(commandList);

		commandAllocator->Release();

		return fenceValue;
	}

	void CommandQueue::ExecuteAndFlush(ID3D12GraphicsCommandList* const commandList)
	{
		uint64_t signalValue = ExecuteCommandList(commandList);
		FlushQueue();
	}

	// Return fence value to for signal.
	uint64_t CommandQueue::Signal()
	{
		uint64_t fenceValueForSignal = ++m_FenceValue;
		ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fenceValueForSignal));

		return fenceValueForSignal;
	}

	bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
	{
		return m_Fence->GetCompletedValue() >= fenceValue;
	}

	void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
	{
		if (!IsFenceComplete(fenceValue))
		{
			ThrowIfFailed(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
			::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
		}
	}

	void CommandQueue::FlushQueue()
	{
		uint64_t fenceValue = Signal();
		WaitForFenceValue(fenceValue);
	}

	wrl::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
	{
		wrl::ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(m_Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));

		return commandAllocator;
	}

	wrl::ComPtr<ID3D12GraphicsCommandList> CommandQueue::CreateCommandList(ID3D12CommandAllocator* commandAllocator)
	{
		wrl::ComPtr<ID3D12GraphicsCommandList> commandList;
		ThrowIfFailed(m_Device->CreateCommandList(0u, m_CommandListType, commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

		return commandList;
	}
}