#pragma once

#include "DescriptorHeap.hpp"

namespace helios::gfx
{
    struct BackBuffer
    {
        wrl::ComPtr<ID3D12Resource> backBufferResource;
        DescriptorHandle backBufferDescriptorHandle;

        ID3D12Resource* const GetResource()
        {
            return backBufferResource.Get();
        }

        std::wstring bufferName{};
    };

    struct Shader
    {
        wrl::ComPtr<IDxcBlob> shaderBlob{};
        wrl::ComPtr<IDxcBlob> rootSignatureBlob{};
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
        std::vector<DXGI_FORMAT> rtvFormats{DXGI_FORMAT_R16G16B16A16_FLOAT};
        uint32_t rtvCount{1u};
        DXGI_FORMAT depthFormat{DXGI_FORMAT_D32_FLOAT};
        D3D12_COMPARISON_FUNC depthComparisonFunc{D3D12_COMPARISON_FUNC_LESS};
        FrontFaceWindingOrder frontFaceWindingOrder{FrontFaceWindingOrder::ClockWise};
        D3D12_CULL_MODE cullMode{D3D12_CULL_MODE_BACK};
        std::wstring pipelineName{};
    };

    struct ComputePipelineStateCreationDesc
    {
        std::wstring_view csShaderPath{};
        std::wstring pipelineName{};
    };
}