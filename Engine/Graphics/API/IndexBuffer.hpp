#pragma once

#include "Pch.hpp"

#include "MemoryAllocator.hpp"

namespace helios::gfx
{
	class Device;

	class IndexBuffer
	{
	public:
		void Init(Device* device, MemoryAllocator* const memoryAllocator, std::span<const uint32_t> data, std::wstring_view indexBufferName);

		D3D12_INDEX_BUFFER_VIEW GetBufferView() const { return mBufferView; }

	private:
		std::unique_ptr<Allocation> mAllocation{};

		D3D12_INDEX_BUFFER_VIEW mBufferView{};
	};
}

