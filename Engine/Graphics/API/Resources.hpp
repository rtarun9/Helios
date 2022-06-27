#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	// Struct with back buffer data.
	struct BackBuffer
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBufferResource;
		gfx::DescriptorHandle backBufferDescriptorHandle;

		ID3D12Resource* GetResource() { return backBufferResource.Get(); }
	};

	// When custom allocator is used much more data will be stored in the allocation struct.
	// Note that the mapped pointer will only be used by constant buffers.
	// The memory allocator class provides methods to create an allocation.
	struct Allocation
	{
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation{};
		void* mappedPointer{};
		Microsoft::WRL::ComPtr<ID3D12Resource> resource{};
		
		void Update(const void* data, size_t size)
		{
			if (!data || !mappedPointer)
			{
				throw std::exception("Trying to update resource that is not placed in CPU visible memory.");
			}

			memcpy(mappedPointer, data, size);
		}
	};

	// Buffer related functions / enum's.
	// Vertex buffer's are not used in the engine. Rather vertex pulling is used and data is stored in structured buffer.
	enum class BufferUsage
	{
		UploadBuffer,
		IndexBuffer,
		StructuredBuffer,
		ConstantBuffer,
	};

	struct BufferCreationDesc
	{
		BufferUsage usage{};
		size_t numComponenets{};
		size_t stride{};
		std::wstring name{};
	};

	struct Buffer
	{
		std::unique_ptr<Allocation> allocation{};
		uint32_t srvCbvIndex{};
		size_t sizeInBytes{};

		// To be used primarily for constant buffers.
		void Update(const void* data, uint32_t size)
		{
			allocation->Update(data, size);
		}
	};

	// Texture related functions / enums.
	// The Depth stencil texture will not have a seperate abstraction and will be created using the common CreateTexture function.
	// Similarly, Render Targets will also be of type Texture.
	enum class TextureUsage
	{
		DepthStencil,
		RenderTarget
	};

	struct TextureCreationDesc
	{
		TextureUsage usage;
		Uint2 dimensions{};
		DXGI_FORMAT format{};
		std::wstring name{};
	};
	
	struct Texture
	{
		std::unique_ptr<Allocation> allocation{};
		uint32_t srvIndex{};
		uint32_t dsvIndex{};
	};

	// Needs to passed to the memory allocator's create buffer function along with a buffer creation desc struct.
	struct ResourceCreationDesc
	{
		D3D12_RESOURCE_DESC resourceDesc{};
	};

	struct SrvCreationDesc
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	};

	struct CbvCreationDesc
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	};

	struct DsvCreationDesc
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	};

	struct RtvCreationDesc
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	};

	// Structs related to pipeline's.
	struct GraphicsPipelineStateCreationDesc
	{
		std::wstring_view vsShaderPath{};
		std::wstring_view psShaderPath{};
		DXGI_FORMAT rtvFormat{ DXGI_FORMAT_R16G16B16A16_FLOAT };
		DXGI_FORMAT depthFormat{ DXGI_FORMAT_D32_FLOAT };
		D3D12_COMPARISON_FUNC depthComparisonFunc{ D3D12_COMPARISON_FUNC_LESS };
		std::wstring pipelineName{};
	};
	
	struct ComputePipelineStateCreationDesc
	{
		std::wstring_view csShaderPath{};
		std::wstring pipelineName{};
	};

	struct RenderTargetDesc
	{
		DXGI_FORMAT format;
		uint32_t width{};
		uint32_t height{};
	};

	struct DepthStencilBufferCreationDesc
	{
		DXGI_FORMAT format{};
		Uint2 dimensions{};
		std::wstring bufferName{};
	};
}