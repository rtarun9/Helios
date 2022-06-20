#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	// When custom allocator is used much more data will be stored in the allocation struct.
	// Note that the mapped pointer will only be used by constant buffers, hence made std::optional.
	struct Allocation
	{
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation{};
		std::optional<void*> mappedPointer{};
		Microsoft::WRL::ComPtr<ID3D12Resource> resource{};
		
		template <typename T>
		void Update(std::span<const T> data)
		{
			memcpy(mappedPointer.value(), data.data(), data.size_bytes());
		}
	};

	// Memory allocator handles allocation of GPU memory. As of now, D3D12 memory allocator is used.
	// note (rtarun9) : The plan is to write a custom allocator in the future, but D3D12MA will be used for now.
	class MemoryAllocator
	{
	public:
		MemoryAllocator(ID3D12Device* device, IDXGIAdapter* adapter);
	
		std::unique_ptr<Allocation> CreateResource(const ResourceCreationDesc& resourceCreationDesc, std::wstring_view resourceName);

	private:
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> mAllocator{};
	};

}
