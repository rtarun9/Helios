#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	struct CommandAllocator
	{
		uint64_t fenceValue{};
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	// Note : The command allocator and command list are not named since these are for internal use only.
	// This might change in the future.
	class CommandQueue
	{
	public:
		void Init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT, std::wstring_view commandQueueName = L"Main Command Queue");

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList();

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

		// Returns the fence value to wait for to notify when command list has finished execution.
		[[nodiscard]]
		uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* commandList);

		// Return fence value to for signal.
		[[nodiscard]]
		uint64_t Signal();
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFenceValue(uint64_t fenceValue);
		void FlushQueue();

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12CommandAllocator* commandAllocator);

	private:
		D3D12_COMMAND_LIST_TYPE m_CommandListType{D3D12_COMMAND_LIST_TYPE_DIRECT};

		// Since the command queue will interact with the ID3D12Device interface, a reference to it is stored here.
		// Because of ComPtr's ref counting mechanism, this should not cause any problems.
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
		
		Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
		HANDLE m_FenceEvent{};
		uint64_t m_FenceValue{};

		std::queue<CommandAllocator> m_CommandAllocatorQueue{};
		std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> m_CommandListQueue{};

	};
}