#pragma once



#include "Descriptor.hpp"

#include "Common/BindlessRS.hlsli"

namespace helios::gfx
{
	// This design choice of having a common Texture / Buffer struct for all various types is inspired from SanityEngine : https://github.com/DethRaid/SanityEngine.
	// Struct with back buffer data.
	struct BackBuffer
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBufferResource;
		gfx::DescriptorHandle backBufferDescriptorHandle;

		ID3D12Resource* GetResource() { return backBufferResource.Get(); }
		std::wstring bufferName{};
	};

	// When custom allocator is used much more data will be stored in the allocation struct.
	// Note that the mapped pointer will only be used by constant buffers, which is why it is wrapped over std::optional.
	// The memory allocator class provides methods to create an allocation.
	struct Allocation
	{		
		Allocation() = default;

		Allocation(const Allocation& other);
		Allocation& operator=(const Allocation& other);

		Allocation(Allocation&& other) noexcept;
		Allocation& operator=(Allocation&& other) noexcept;

		void Update(const void* data, size_t size);
		void Reset();

		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation{};
		std::optional<void*> mappedPointer{};
		Microsoft::WRL::ComPtr<ID3D12Resource> resource{};
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
		// To be used primarily for constant buffers.
		void Update(const void* data);

		// In the model abstraction, the buffers are wrapped in unique pointers.
		// Due to this, we cant access any of the indices if the buffer is nullptr.
		// So, upon passing the buffer to this function, it will return INVALID_INDEX (UINT32_MAX) is buffer is null, or the index otherwise.
		static uint32_t GetSrvIndex(const Buffer* buffer);
		static uint32_t GetCbvIndex(const Buffer* buffer);
		static uint32_t GetUavIndex(const Buffer* buffer);

		std::unique_ptr<Allocation> allocation{};
		std::wstring bufferName{};
		size_t sizeInBytes{};

	private:
		// These are made private so that if a particular buffer does not exist, we set the index as INVALID_INDEX, which the shader recognizes and takes proper action.
		// Access these using the GetXIndex calls.
		uint32_t srvIndex{};
		uint32_t uavIndex{};
		uint32_t cbvIndex{};

		// Required as device sets all the indices.
		friend class Device;
	};

	// Texture related functions / enum's.
	// The Depth stencil texture will not have a separate abstraction and will be created using the common CreateTexture function.
	// Similarly, Render Targets will also be of type Texture.
	// TextureUpload is used for intermediate buffers (as used in UpdateSubresources).
	// If data is already loaded elsewhere, use the TextureFromData enum (this requires TextureCreateionDesc has all properties correctly set (specifically dimensions).
	enum class TextureUsage
	{
		DepthStencil,
		RenderTarget,
		TextureFromPath,	
		TextureFromData,
		HDRTextureFromPath,
		CubeMap
	};

	struct TextureCreationDesc
	{
		TextureUsage usage;
		Uint2 dimensions{};
		DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
		uint32_t mipLevels{ 1u };
		uint32_t depthOrArraySize{ 1u };
		std::wstring name{};
		std::wstring path{};
	};
	
	struct Texture
	{
		ID3D12Resource* const GetResource() const;

		// In the model abstraction, the textures are wrapped in unique pointers.
		// Due to this, we cant access any of the indices if the pointer is nullptr.
		// So, upon passing the texture to this function, it will return UINT32_MAX / INVALID_INDEX if texture is null, or the srvIndex otherwise.
		static uint32_t GetSrvIndex(const Texture* texture);
		static uint32_t GetUavIndex(const Texture* texture);
		static uint32_t GetDsvIndex(const Texture* texture);
		static uint32_t GetRtvIndex(const Texture* texture);

		static bool IsTextureSRGB(const DXGI_FORMAT& format);
		static DXGI_FORMAT GetNonSRGBFormat(const DXGI_FORMAT& format);

		std::wstring textureName{};
		Uint2 dimensions{};
		std::unique_ptr<Allocation> allocation{};

	private:
		uint32_t srvIndex{};
		uint32_t uavIndex{};
		uint32_t dsvIndex{};
		uint32_t rtvIndex{};

		friend class Device;
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

	struct UavCreationDesc
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	};

	// Struct's related to pipeline's.
	struct ShaderModule
	{
		std::wstring vsShaderPath{};
		std::wstring psShaderPath{};
	};

	// Winding order will always be clockwise except for cube maps, where we want to see the inner faces of cube map.
	// So, the default winding order will be clockwise, and for cube maps it has to be set to counter clock wise.
	enum class FrontFaceWindingOrder
	{
		ClockWise,
		CounterClockWise
	};

	struct GraphicsPipelineStateCreationDesc
	{
		ShaderModule shaderModule{};
		std::vector<DXGI_FORMAT> rtvFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT };
		DXGI_FORMAT depthFormat{ DXGI_FORMAT_D32_FLOAT };
		D3D12_COMPARISON_FUNC depthComparisonFunc{ D3D12_COMPARISON_FUNC_LESS };
		FrontFaceWindingOrder frontFaceWindingOrder{ FrontFaceWindingOrder::ClockWise };
		uint32_t rtvCount{ 1u };
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

	// Render target abstraction is created so that all off screen render targets can share the same index buffer / texture coords etc.
	// Note : the destroy function has to be called from the device so that the D3D12MA::Allocation objects(s) can be cleared properly.
	class Device;
	class GraphicsContext;

	struct RenderTarget
	{
		// Create all buffers.
		static void CreateRenderTargetResources(const Device* device);
		
		// Destroy all buffers (as they have D3D12MA::Allocation objects).
		static void DestroyResources();

		// Getters.
		static uint32_t GetPositionBufferIndex() { return Buffer::GetSrvIndex(sPositionBuffer.get()); }
		static uint32_t GetTextureCoordsBufferIndex() { return Buffer::GetSrvIndex(sTextureCoordsBuffer.get()); }

		static uint32_t GetRenderTextureRTVIndex(const RenderTarget* renderTarget);
		static uint32_t GetRenderTextureSRVIndex(const RenderTarget* renderTarget);

		static void Render(const GraphicsContext* graphicsContext, RenderTargetRenderResources& renderTargetRenderResources);
		static void Render(const GraphicsContext* graphicsContext, DeferredLightingPassRenderResources& deferredLightingRenderResources);

		static inline std::unique_ptr<Buffer> sIndexBuffer;
		static inline std::unique_ptr<Buffer> sPositionBuffer;
		static inline std::unique_ptr<Buffer> sTextureCoordsBuffer;
		
		ID3D12Resource* const GetResource() const { return renderTexture->GetResource(); }

		std::wstring renderTargetName{};
		std::unique_ptr<Texture> renderTexture{};
	};

	// Each texture (i.e loaded from a GLTF file) will have a sampler associated with it.
	struct SamplerCreationDesc
	{
		D3D12_SAMPLER_DESC samplerDesc{};
	};
}