#include "Pch.hpp"

#include "Resources.hpp"
#include "Device.hpp"

namespace helios::gfx
{
	Allocation::Allocation(const Allocation& other) : resource(other.resource), allocation(other.allocation)
	{
		if (other.mappedPointer.has_value())
		{
			mappedPointer = other.mappedPointer;
		}
	}

	Allocation& Allocation::operator=(const Allocation& other)
	{
		if (&other == this)
		{
			return *this;
		}

		// Clear state of current object.
		resource->Release();
		allocation->Release();

		if (other.mappedPointer.has_value())
		{
			mappedPointer = other.mappedPointer;
		}

		resource = other.resource;		
		allocation = other.allocation;

		return *this;
	}

	Allocation::Allocation(Allocation&& other) noexcept
		: resource(std::move(other.resource)), allocation(std::move(other.allocation))
	{
		if (other.mappedPointer.has_value())
		{
			mappedPointer = other.mappedPointer;
		}
	}

	Allocation& Allocation::operator=(Allocation&& other) noexcept
	{
		resource = std::move(other.resource); 
		allocation = std::move(other.allocation);
	
		if (other.mappedPointer.has_value())
		{
			mappedPointer = other.mappedPointer;
		}

		return *this;
	}

	void Allocation::Update(const void* data, size_t size)
	{
		if (!data || !mappedPointer.has_value())
		{
			throw std::exception("Trying to update resource that is not placed in CPU visible memory, or the data is null");
		}

		memcpy(mappedPointer.value(), data, size);
	}

	void Allocation::Reset()
	{
		resource.Reset();
		allocation.Reset();
	}

	// To be used primarily for constant buffers.
	void Buffer::Update(const void* data)
	{
		// Allocation's update function will take care if data is null or not.
		allocation->Update(data, sizeInBytes);
	}

	// In the model abstraction, the buffers are wrapped in unique pointers.
	// Due to this, we cant access any of the indices if the buffer is nullptr.
	// So, upon passing the buffer to this function, it will return -1 is buffer is null, or the index otherwise.
	uint32_t Buffer::GetSrvIndex(const Buffer* buffer)
	{
		if (buffer == nullptr)
		{
			return UINT32_MAX;
		}

		return buffer->srvIndex;
	}

	uint32_t Buffer::GetCbvIndex(const Buffer* buffer)
	{
		if (buffer == nullptr)
		{
			return UINT32_MAX;
		}

		return buffer->cbvIndex;
	}

	uint32_t Buffer::GetUavIndex(const Buffer* buffer)
	{
		if (buffer == nullptr)
		{
			return UINT32_MAX;
		}

		return buffer->uavIndex;
	}


	ID3D12Resource* const Texture::GetResource() const
	{
		if (allocation)
		{
			return allocation->resource.Get();
		}

		return nullptr;
	}

	// In the model abstraction, the textures are wrapped in unique pointers.
	// Due to this, we cant access any of the indices if the pointer is nullptr.
	// So, upon passing the texture to this function, it will return UINT32_MAX / INVALID_INDEX if texture is null, or the srvIndex otherwise.
	uint32_t Texture::GetSrvIndex(const Texture* texture)
	{
		if (texture == nullptr)
		{
			return UINT32_MAX;
		}

		return texture->srvIndex;
	}

	uint32_t Texture::GetUavIndex(const Texture* texture)
	{
		if (texture == nullptr)
		{
			return UINT32_MAX;
		}

		return texture->uavIndex;
	}

	uint32_t Texture::GetDsvIndex(const Texture* texture)
	{
		if (texture == nullptr)
		{
			return UINT32_MAX;
		}

		return texture->dsvIndex;
	}

	uint32_t Texture::GetRtvIndex(const Texture* texture)
	{
		if (texture == nullptr)
		{
			return UINT32_MAX;
		}

		return texture->rtvIndex;
	}

	bool Texture::IsTextureSRGB(const DXGI_FORMAT& format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		{
			return true;
		}break;

		default:
		{
			return false;
		}break;
		}
	};

	DXGI_FORMAT Texture::GetNonSRGBFormat(const DXGI_FORMAT& format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		{
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}break;

		case DXGI_FORMAT_BC1_UNORM_SRGB:
		{
			return DXGI_FORMAT_BC1_UNORM;
		}break;

		case DXGI_FORMAT_BC2_UNORM_SRGB:
		{
			return DXGI_FORMAT_BC2_UNORM;
		}break;

		case DXGI_FORMAT_BC3_UNORM_SRGB:
		{
			return DXGI_FORMAT_BC3_UNORM;
		}break;

		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}break;

		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		{
			return DXGI_FORMAT_B8G8R8X8_UNORM;
		}break;

		case DXGI_FORMAT_BC7_UNORM_SRGB:
		{
			return DXGI_FORMAT_BC7_UNORM;
		}break;

		default:
		{
			return format;
		}break;
		}
	}

	void RenderTarget::CreateRenderTargetResources(const Device* device)
	{
		// Buffer data.
		static constexpr std::array<DirectX::XMFLOAT2, 4> RT_VERTEX_POSITIONS
		{
			DirectX::XMFLOAT2(-1.0f, -1.0f),
			DirectX::XMFLOAT2(-1.0f,   1.0f),
			DirectX::XMFLOAT2(1.0f, 1.0f),
			DirectX::XMFLOAT2(1.0f, -1.0f),
		};

		static constexpr std::array<DirectX::XMFLOAT2, 4> RT_VERTEX_TEXTURE_COORDS
		{
			DirectX::XMFLOAT2(0.0f, 1.0f),
			DirectX::XMFLOAT2(0.0f, 0.0f),
			DirectX::XMFLOAT2(1.0f, 0.0f),
			DirectX::XMFLOAT2(1.0f, 1.0f),
		};

		static constexpr std::array<uint32_t, 6> RT_INDICES
		{
			0u, 1u, 2u,
			0u, 2u, 3u
		};

		// Create index buffer.
		static const gfx::BufferCreationDesc INDEX_BUFFER_CREATION_DESC
		{
			.usage = gfx::BufferUsage::IndexBuffer,
			.name = L"Render Target Index Buffer",
		};

		sIndexBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<uint32_t>(INDEX_BUFFER_CREATION_DESC, RT_INDICES));

		// Create vertex buffer.
		static const gfx::BufferCreationDesc POSITION_BUFFER_CREATION_DESC
		{
			.usage = gfx::BufferUsage::StructuredBuffer,
			.name = L"Render Target Position Buffer",
		};

		sPositionBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT2>(POSITION_BUFFER_CREATION_DESC, RT_VERTEX_POSITIONS));

		// Create Texture coords buffer.
		gfx::BufferCreationDesc TEXTURE_COORDS_BUFFER_CREATION_DESC
		{
			.usage = gfx::BufferUsage::StructuredBuffer,
			.name = L"Render Target Texture Coords Buffer",
		};

		sTextureCoordsBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT2>(TEXTURE_COORDS_BUFFER_CREATION_DESC, RT_VERTEX_TEXTURE_COORDS));
	}

	// Destroy all buffers (as they have D3D12MA::Allocation objects). 
	// Must be called from the device, before the ID3D12Device* and MemoryAllocator go out of scope.
	void RenderTarget::DestroyResources()
	{
		sPositionBuffer->allocation->Reset();
		sIndexBuffer->allocation->Reset();
		sTextureCoordsBuffer->allocation->Reset();
	}

	void RenderTarget::Render(const GraphicsContext* graphicsContext, RenderTargetRenderResources& renderTargetRenderResources)
	{
		renderTargetRenderResources.positionBufferIndex = Buffer::GetSrvIndex(sPositionBuffer.get());
		renderTargetRenderResources.textureBufferIndex = Buffer::GetSrvIndex(sTextureCoordsBuffer.get());

		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsContext->SetIndexBuffer(RenderTarget::sIndexBuffer.get());
		
		graphicsContext->Set32BitGraphicsConstants(&renderTargetRenderResources);
		
		graphicsContext->DrawInstanceIndexed(6u);
	}

	void RenderTarget::Render(const GraphicsContext* graphicsContext, DeferredLightingPassRenderResources& deferredLightingRenderResources)
	{
		deferredLightingRenderResources.positionBufferIndex = Buffer::GetSrvIndex(sPositionBuffer.get());
		deferredLightingRenderResources.textureBufferIndex = Buffer::GetSrvIndex(sTextureCoordsBuffer.get());

		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsContext->SetIndexBuffer(RenderTarget::sIndexBuffer.get());

		graphicsContext->Set32BitGraphicsConstants(&deferredLightingRenderResources);

		graphicsContext->DrawInstanceIndexed(6u);
	}

	uint32_t RenderTarget::GetRenderTextureRTVIndex(const RenderTarget* renderTarget)
	{
		if (!renderTarget)
		{
			return UINT32_MAX;
		}

		return Texture::GetRtvIndex(renderTarget->renderTexture.get());
	}

	uint32_t RenderTarget::GetRenderTextureSRVIndex(const RenderTarget* renderTarget)
	{
		if (!renderTarget)
		{
			return UINT32_MAX;
		}

		return Texture::GetSrvIndex(renderTarget->renderTexture.get());
	}
}