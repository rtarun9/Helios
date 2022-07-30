#include "Pch.hpp"

#include "Resources.hpp"
#include "Device.hpp"

namespace helios::gfx
{
	// Unlike other resources the device will not be in control of creating rendertargets.
	// Done because render target is a collection of various resources.
	RenderTarget::RenderTarget(const Device* device, const TextureCreationDesc& textureCreationDesc)
	{
		renderTexture = std::make_unique<gfx::Texture>(device->CreateTexture(textureCreationDesc));
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
		gfx::BufferCreationDesc indexBufferCreationDesc
		{
			.usage = gfx::BufferUsage::IndexBuffer,
			.name = L"Render Target Index Buffer",
		};

		sIndexBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<uint32_t>(indexBufferCreationDesc, RT_INDICES));

		// Create vertex buffer.
		gfx::BufferCreationDesc positionBufferCreationDesc
		{
			.usage = gfx::BufferUsage::StructuredBuffer,
			.name = L"Render Target Position Buffer",
		};

		sPositionBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT2>(positionBufferCreationDesc, RT_VERTEX_POSITIONS));

		// Create Texture coords buffer.
		gfx::BufferCreationDesc textureCoordsBufferCreationDesc
		{
			.usage = gfx::BufferUsage::StructuredBuffer,
			.name = L"Render Target Texture Coords Buffer",
		};

		sTextureCoordsBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT2>(textureCoordsBufferCreationDesc, RT_VERTEX_TEXTURE_COORDS));
	}

	// Destroy all buffers (as they have D3D12MA::Allocation objects). 
	// Must be called from the device, before the ID3D12Device* and MemoryAllocator go out of scope.
	void RenderTarget::DestroyResources()
	{
		sPositionBuffer->allocation->Reset();
		sIndexBuffer->allocation->Reset();
		sTextureCoordsBuffer->allocation->Reset();
	}

	void RenderTarget::Draw(const GraphicsContext* graphicsContext, RenderTargetRenderResources& renderTargetRenderResources)
	{
		renderTargetRenderResources.positionBufferIndex = sPositionBuffer->srvIndex;
		renderTargetRenderResources.textureBufferIndex = sTextureCoordsBuffer->srvIndex;

		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsContext->SetIndexBuffer(RenderTarget::sIndexBuffer.get());
		
		graphicsContext->Set32BitGraphicsConstants(&renderTargetRenderResources);
		
		graphicsContext->DrawInstanceIndexed(6u);
	}
}