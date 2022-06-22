#pragma once

#include "Pch.hpp"

#include "CommandQueue.hpp"
#include "MemoryAllocator.hpp"

// note : The following set of abstractions are inspired by Wicked Engine's allocation approach.
// link : https://github.com/turanszkij/WickedEngine/blob/master/WickedEngine/wiGraphicsDevice_DX12.cpp.
namespace helios::gfx
{
	class Device;

	// The allocation object holds the intermediate buffer (GPU only) that the command list will 
	struct UploadAllocation
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList{};
		std::unique_ptr<Allocation> mAllocation{};
		UploadBufferDesc uploadBufferDesc{};

		// The destination resource has valued copied from mAllocation->mResource, the source resource in this case.
		void CopyResource(Allocation* destination)
		{
			commandList->CopyResource(destination->resource.Get(), mAllocation->resource.Get());
		}

		template <typename T>
		void Update(std::span<const T> data)
		{
			memcpy(mAllocation->mappedPointer.value(), data.data(), data.size_bytes());
		}
	};

	// Wrapper class for CommandList used primarily for copying data from CPU to GPU only memory by creating an intermediate buffer.
	// Provides of easy and simple functions to record commands for execution by GPU.
	// It holds the copy command queue within itself (unlike the Graphics Command Queue).
	// note (rtarun9) : This design is subject to change.
	class UploadContext
	{
	public:
		UploadContext(ID3D12Device5* const device, MemoryAllocator*  memoryAllocator);

		// Core functionalities.
		std::unique_ptr<UploadAllocation> Allocate(uint32_t bufferSize);
		void ProcessUploadAllocations(std::unique_ptr<UploadAllocation> uploadAllocations);

		~UploadContext();

	private:
		std::unique_ptr<CommandQueue> mUploadCommandQueue{};
		std::vector<UploadAllocation> mUploadAllocations{};

		// Has a reference to the memory allocator, as new buffer will be created if we cannot find a copy allocator to fit the requested buffer size.
		std::shared_ptr<MemoryAllocator> mMemoryAllocator;
	};
}


