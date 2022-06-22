#include "Pch.hpp"

#include "UploadContext.hpp"

namespace helios::gfx
{
	UploadContext::UploadContext(ID3D12Device5* const device, MemoryAllocator* memoryAllocator)
	{
		mMemoryAllocator = std::make_shared<MemoryAllocator>(*memoryAllocator);
		mUploadCommandQueue = std::make_unique<CommandQueue>(device, D3D12_COMMAND_LIST_TYPE_COPY, L"Upload Context Command Queue");
	}

	std::unique_ptr<UploadAllocation> UploadContext::Allocate(uint32_t bufferSize)
	{
		// If no allocators exist, get a new command list from the copy command queue.
		if (mUploadAllocations.empty())
		{
			UploadAllocation copyAllocator =
			{
				.commandList = mUploadCommandQueue->GetCommandList(),
				.mAllocation{},
				.uploadBufferDesc{}
			};
		}

		UploadAllocation copyAllocator = std::move(mUploadAllocations.back());

		// If the current copy allocator does not have a staging buffer that can fit the bufferSize that was requested, find one if it exist (first fit).
		if (copyAllocator.uploadBufferDesc.bufferSize < bufferSize)
		{
			for (auto& uploadAllocations : mUploadAllocations)
			{
				if (uploadAllocations.uploadBufferDesc.bufferSize >= bufferSize)
				{
					std::swap(uploadAllocations, mUploadAllocations.back());
					copyAllocator = std::move(uploadAllocations);
					break;
				}
			}
		}

		mUploadAllocations.pop_back();

		// If, however no buffer was found, create a new one.
		if (copyAllocator.uploadBufferDesc.bufferSize < bufferSize)
		{
			ResourceCreationDesc uploadBufferDesc
			{
				.size = bufferSize,
				.isCPUVisible = true,
			};

			copyAllocator.mAllocation = mMemoryAllocator->CreateResource(uploadBufferDesc, L"Intermediate copy allocator");
		}

		return std::move(std::make_unique<UploadAllocation>(copyAllocator));
	}

	void UploadContext::ProcessUploadAllocations(std::unique_ptr<UploadAllocation> copyAllocations)
	{
		mUploadCommandQueue->ExecuteAndFlush(copyAllocations->commandList.Get());
		copyAllocations->commandList.Reset();
		mUploadAllocations.push_back(std::move(*copyAllocations));
	}

	UploadContext::~UploadContext()
	{
		mUploadCommandQueue->FlushQueue();
	}
}

