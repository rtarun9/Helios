#pragma once

#include "DescriptorHeap.hpp"

namespace helios::gfx
{
    struct Shader
    {
        wrl::ComPtr<IDxcBlob> shaderBlob{};
        wrl::ComPtr<IDxcBlob> rootSignatureBlob{};
    };

    // Struct's related to pipeline's.
    struct ShaderModule
    {
        std::wstring_view vertexShaderPath{};
        std::wstring_view vertexEntryPoint{L"VsMain"};

        std::wstring_view pixelShaderPath{};
        std::wstring_view pixelEntryPoint{L"PsMain"};

        std::wstring_view computeShaderPath{};
        std::wstring_view computeEntryPoint{L"CsMain"};
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
        std::vector<DXGI_FORMAT> rtvFormats{DXGI_FORMAT_R10G10B10A2_UNORM};
        uint32_t rtvCount{1u};
        DXGI_FORMAT depthFormat{DXGI_FORMAT_D32_FLOAT};
        D3D12_COMPARISON_FUNC depthComparisonFunc{D3D12_COMPARISON_FUNC_LESS};
        FrontFaceWindingOrder frontFaceWindingOrder{FrontFaceWindingOrder::ClockWise};
        D3D12_CULL_MODE cullMode{D3D12_CULL_MODE_BACK};
        std::wstring_view pipelineName{};
    };

    struct ComputePipelineStateCreationDesc
    {
        std::wstring_view csShaderPath{};
        std::wstring_view pipelineName{};
    };

    // Resource related structs.
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

    struct SamplerCreationDesc
    {
        D3D12_SAMPLER_DESC samplerDesc{};
    };

    // Note that the mapped pointer will only be used by constant buffers, which is why it is wrapped over
    // std::optional. The memory allocator class provides methods to create an allocation.
    struct Allocation
    {
        Allocation() = default;

        Allocation(const Allocation& other);
        Allocation& operator=(const Allocation& other);

        Allocation(Allocation&& other) noexcept;
        Allocation& operator=(Allocation&& other) noexcept;

        void update(const void* data, const size_t size);
        void reset();

        wrl::ComPtr<D3D12MA::Allocation> allocation{};
        std::optional<void*> mappedPointer{};
        wrl::ComPtr<ID3D12Resource> resource{};
    };

    // Buffer related functions / enum's.
    // Vertex buffer's are not used in the engine. Rather vertex pulling is used and data is stored in structured
    // buffer.
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
        std::wstring_view name{};
    };

    struct Buffer
    {
        // To be used primarily for constant buffers.
        void update(const void* data);

        Allocation allocation{};
        size_t sizeInBytes{};

        uint32_t srvIndex{INVALID_INDEX_U32};
        uint32_t uavIndex{INVALID_INDEX_U32};
        uint32_t cbvIndex{INVALID_INDEX_U32};
    };

    // Needs to passed to the memory allocator's create buffer function along with a buffer creation desc struct.
    struct ResourceCreationDesc
    {
        D3D12_RESOURCE_DESC resourceDesc{};

        [[nodiscard]] static ResourceCreationDesc createBufferResourceCreationDesc(const uint64_t size)
        {
            ResourceCreationDesc resourceCreationDesc = {
                .resourceDesc{
                    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                    .Width = size,
                    .Height = 1u,
                    .DepthOrArraySize = 1u,
                    .MipLevels = 1u,
                    .Format = DXGI_FORMAT_UNKNOWN,
                    .SampleDesc{.Count = 1u, .Quality = 0u},
                    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                    .Flags = D3D12_RESOURCE_FLAG_NONE,
                },
            };

            return resourceCreationDesc;
        }
    };

    // Texture related functions / enum's.
    // The Depth stencil texture will not have a separate abstraction and will be created using the common CreateTexture
    // function. Similarly, Render Targets will also be of type Texture. TextureUpload is used for intermediate buffers
    // (as used in UpdateSubresources). If data is already loaded elsewhere, use the TextureFromData enum (this requires
    // TextureCreateionDesc has all properties correctly set (specifically dimensions). UAV Texture is just a regular
    // texture with flags to allow it to be used as a UAV.
    // note(rtarun9) : These usages correspond more with how textures are being created then how they are being used,
    // consider changing this in the future.
    enum class TextureUsage
    {
        DepthStencil,
        RenderTarget,
        TextureFromPath,
        TextureFromData,
        HDRTextureFromPath,
        CubeMap,
        UAVTexture
    };

    struct TextureCreationDesc
    {
        TextureUsage usage{};
        uint32_t width{};
        uint32_t height{};
        DXGI_FORMAT format{DXGI_FORMAT_R8G8B8A8_UNORM};
        D3D12_RESOURCE_STATES optionalInitialState{D3D12_RESOURCE_STATE_COMMON};
        uint32_t mipLevels{1u};
        uint32_t depthOrArraySize{1u};
        std::wstring_view name{};
        std::wstring path{};
    };

    struct Texture
    {
        uint32_t width{};
        uint32_t height{};
        Allocation allocation{};

        uint32_t srvIndex{INVALID_INDEX_U32};
        uint32_t uavIndex{INVALID_INDEX_U32};
        uint32_t dsvIndex{INVALID_INDEX_U32};
        uint32_t rtvIndex{INVALID_INDEX_U32};

        static bool isTextureSRGB(const DXGI_FORMAT format);
        static DXGI_FORMAT getNonSRGBFormat(const DXGI_FORMAT format);
    };

    struct Sampler
    {
        uint32_t samplerIndex{INVALID_INDEX_U32};
    };
} // namespace helios::gfx