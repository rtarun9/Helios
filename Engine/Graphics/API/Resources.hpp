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
	// Note that the mapped pointer will only be used by constant buffers, which is why it is wrapped over std::optional.
	// The memory allocator class provides methods to create an allocation.
	struct Allocation
	{
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation{};
		std::optional<void*> mappedPointer{};
		Microsoft::WRL::ComPtr<ID3D12Resource> resource{};
		
		void Update(const void* data, size_t size)
		{
			if (!data || !mappedPointer.has_value())
			{
				throw std::exception("Trying to update resource that is not placed in CPU visible memory, or the data is null");
			}

			memcpy(mappedPointer.value(), data, size);
		}

		void Reset()
		{
			resource.Reset();
			allocation.Reset();
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
		std::wstring name{};
	};

	struct Buffer
	{
		std::unique_ptr<Allocation> allocation{};
		uint32_t srvCbvUavIndex{};
		size_t sizeInBytes{};

		// To be used primarily for constant buffers.
		void Update(const void* data)
		{
			// Allocation's update function will take care if data is null or not.
			allocation->Update(data, sizeInBytes);
		}

		// In the model abstraction, the buffers are wrapped in unique pointers.
		// Due to this, we cant access any of the indices if the buffer is nullptr.
		// So, upon passing the buffer to this function, it will return 0 is buffer is null, or the srvCbvUavIndex otherwise.
		static uint32_t GetSrvCbvUavIndex(const Buffer* buffer)
		{
			if (buffer == nullptr)
			{
				return 0;
			}

			return buffer->srvCbvUavIndex;
		}
	};

	// Texture related functions / enums.
	// The Depth stencil texture will not have a seperate abstraction and will be created using the common CreateTexture function.
	// Similarly, Render Targets will also be of type Texture.
	// TextureUpload is used for intermediate buffers (as used in UpdateSubresources).
	enum class TextureUsage
	{
		DepthStencil,
		RenderTarget,
		TextureFromPath,	
	};

	struct TextureCreationDesc
	{
		TextureUsage usage;
		Uint2 dimensions{};
		DXGI_FORMAT format{DXGI_FORMAT_R8G8B8A8_UNORM};
		uint32_t mipLevels{1u};
		std::wstring name{};
		std::wstring path{};
	};
	
	struct Texture
	{
		std::unique_ptr<Allocation> allocation{};
		uint32_t srvIndex{};
		uint32_t dsvIndex{};

		// In the model abstraction, the textures are wrapped in unique pointers.
		// Due to this, we cant access any of the indices if the pointer is nullptr.
		// So, upon passing the texture to this function, it will return 0 is texture is null, or the srvIndex otherwise.
		static uint32_t GetSrvIndex(const Texture* texture)
		{
			if (texture == nullptr)
			{
				return 0;
			}

			return texture->srvIndex;
		}
	};

	// Needs to passed to the memory allocator's create buffer function along with a buffer creation desc struct.
	struct ResourceCreationDesc
	{
		D3D12_RESOURCE_DESC resourceDesc{};

		static ResourceCreationDesc CreateBufferResourceCreationDesc(uint64_t size)
		{
			ResourceCreationDesc resourceCreationDesc
			{
				.resourceDesc
				{
					.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
					.Width = size,
					.Height = 1u,
					.DepthOrArraySize = 1u,
					.MipLevels = 1u,
					.Format = DXGI_FORMAT_UNKNOWN,
					.SampleDesc
					{
						.Count = 1u,
						.Quality = 0u
					},
					.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
					.Flags = D3D12_RESOURCE_FLAG_NONE
				}
			};

			return resourceCreationDesc;
		}
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