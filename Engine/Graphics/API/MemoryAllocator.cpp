#include "Pch.hpp"

#include "MemoryAllocator.hpp"

#include "Resources.hpp"

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace Microsoft::WRL;

namespace helios::gfx
{
	MemoryAllocator::MemoryAllocator(ID3D12Device* device, IDXGIAdapter* adapter)
	{
		// Create D3D12MA adapter.
		D3D12MA::ALLOCATOR_DESC allocatorDesc
		{
			.pDevice = device,
			.pAdapter = adapter,
		};

		ThrowIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &mAllocator));
	}

	std::unique_ptr<Allocation> MemoryAllocator::CreateBufferResourceAllocation(const BufferCreationDesc& bufferCreationDesc, const ResourceCreationDesc& resourceCreationDesc)
	{
		Allocation allocation{};

		D3D12_RESOURCE_STATES resourceState{D3D12_RESOURCE_STATE_COMMON};
		D3D12_HEAP_TYPE heapType{D3D12_HEAP_TYPE_DEFAULT};
		bool isCpuVisible{};

		switch (bufferCreationDesc.usage)
		{
			case BufferUsage::UploadBuffer:
			case BufferUsage::ConstantBuffer:
			{
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
				heapType = D3D12_HEAP_TYPE_UPLOAD;
				isCpuVisible = true;
			}break;

			case BufferUsage::IndexBuffer:
			case BufferUsage::StructuredBuffer:
			{
				resourceState = D3D12_RESOURCE_STATE_COMMON;
				heapType = D3D12_HEAP_TYPE_DEFAULT;
				isCpuVisible = false;
			}break;
		};

		D3D12MA::ALLOCATION_DESC allocationDesc
		{
			.HeapType = heapType
		};

		ThrowIfFailed(mAllocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState, nullptr, &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));
		
		if (isCpuVisible)
		{
			// Give the mapped pointer some value to hold.
			allocation.mappedPointer = nullptr;
			ThrowIfFailed(allocation.resource->Map(0u, nullptr, &allocation.mappedPointer.value()));
		}

		allocation.resource->SetName(bufferCreationDesc.name.c_str());

		return std::move(std::make_unique<Allocation>(allocation));
	}

	std::unique_ptr<Allocation> MemoryAllocator::CreateTextureResourceAllocation(TextureCreationDesc& textureCreationDesc)
	{
		Allocation allocation{};


		D3D12_RESOURCE_STATES resourceState{ D3D12_RESOURCE_STATE_COMMON };
		D3D12_HEAP_TYPE heapType{ D3D12_HEAP_TYPE_DEFAULT };

		D3D12MA::ALLOCATION_DESC allocationDesc
		{
			.HeapType = heapType
		};

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
		}

		ResourceCreationDesc resourceCreationDesc
		{
			.resourceDesc
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.Alignment = 0u,
				.Width = textureCreationDesc.dimensions.x,
				.Height = textureCreationDesc.dimensions.y,
				.DepthOrArraySize = static_cast<UINT16>(textureCreationDesc.depthOrArraySize),
				.MipLevels = static_cast<UINT16>(textureCreationDesc.mipLevels),
				.Format = format,
				.SampleDesc
				{
					.Count = 1u,
					.Quality = 0u
				},
				.Flags = D3D12_RESOURCE_FLAG_NONE
			}
		};
	
		if (resourceCreationDesc.resourceDesc.MipLevels >= resourceCreationDesc.resourceDesc.Width)
		{
			resourceCreationDesc.resourceDesc.MipLevels = static_cast<UINT16>(resourceCreationDesc.resourceDesc.Width - 1);
		}

		if (resourceCreationDesc.resourceDesc.MipLevels >= resourceCreationDesc.resourceDesc.Height)
		{
			resourceCreationDesc.resourceDesc.MipLevels = static_cast<UINT16>(resourceCreationDesc.resourceDesc.Height- 1);
		}

		textureCreationDesc.mipLevels = resourceCreationDesc.resourceDesc.MipLevels;

		switch (textureCreationDesc.usage)
		{
			case TextureUsage::DepthStencil:
			{
				resourceCreationDesc.resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
				resourceCreationDesc.resourceDesc.Format = dsFormat;
				allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
				resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			}break;

			case TextureUsage::RenderTarget:
			{
				resourceCreationDesc.resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
				allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
				resourceState = D3D12_RESOURCE_STATE_COMMON;
			}break;

			// Note : All resource loaded from path must be able to be used by UAVs.
			case TextureUsage::TextureFromPath:
			case TextureUsage::TextureFromData:
			case TextureUsage::CubeMap:
			{
				resourceCreationDesc.resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				resourceState = D3D12_RESOURCE_STATE_COMMON;
			}break;
		};

		std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{};

		if (textureCreationDesc.usage == TextureUsage::RenderTarget)
		{
			optimizedClearValue = 
			{
					.Format = format,
					.Color = { 0.0f, 0.0f, 0.0f, 1.0f },
			};

		}
		else if (textureCreationDesc.usage == TextureUsage::DepthStencil)
		{
			D3D12_DEPTH_STENCIL_VALUE dsValue
			{
				.Depth = 1.0f,
				.Stencil = 1u
			};

			optimizedClearValue =
			{
					.Format = dsFormat,
					.DepthStencil = dsValue
			};
		}

		ThrowIfFailed(mAllocator->CreateResource(&allocationDesc, &resourceCreationDesc.resourceDesc, resourceState,optimizedClearValue.has_value() ? &optimizedClearValue.value() : nullptr, &allocation.allocation, IID_PPV_ARGS(&allocation.resource)));
		allocation.resource->SetName(textureCreationDesc.name.c_str());

		return std::move(std::make_unique<Allocation>(allocation));
	}
}