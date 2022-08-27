#pragma once



#include "Resources.hpp"

namespace helios::gfx
{
	// Memory allocator handles allocation of GPU memory. As of now, D3D12 memory allocator is used.
	// note (rtarun9) : The plan is to write a custom allocator in the future, but D3D12MA will be used for now.
	class MemoryAllocator
	{
	public:
		MemoryAllocator(ID3D12Device* device, IDXGIAdapter* adapter);
	
		// While both the CreateXResourceAllocation can be merged, it leads to a bit strange / awkward code, so seperating it for now.
		std::unique_ptr<Allocation> CreateBufferResourceAllocation(const BufferCreationDesc& bufferCreationDesc, const ResourceCreationDesc& resourceCreationDesc);
		std::unique_ptr<Allocation> CreateTextureResourceAllocation(TextureCreationDesc& textureCreationDesc);

	private:
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> mAllocator{};
		std::recursive_mutex mResourceAllocationMutex{};
	};

}
