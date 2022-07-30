#include "Pch.hpp"

#include "Resources.hpp"
#include "Device.hpp"

namespace helios::gfx
{
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
		renderTargetRenderResources.positionBufferIndex = sPositionBuffer->srvIndex;
		renderTargetRenderResources.textureBufferIndex = sTextureCoordsBuffer->srvIndex;

		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsContext->SetIndexBuffer(RenderTarget::sIndexBuffer.get());
		
		graphicsContext->Set32BitGraphicsConstants(&renderTargetRenderResources);
		
		graphicsContext->DrawInstanceIndexed(6u);
	}
}