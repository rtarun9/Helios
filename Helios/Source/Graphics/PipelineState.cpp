#include "Graphics/PipelineState.hpp"

#include "Core/ResourceManager.hpp"

namespace helios::gfx
{
    PipelineState::PipelineState(ID3D12Device5* const device,
                                 const GraphicsPipelineStateCreationDesc& pipelineStateCreationDesc)
    {
        const auto vsPath = core::ResourceManager::getAssetsPath(pipelineStateCreationDesc.shaderModule.vsShaderPath);
        const auto psPath = core::ResourceManager::getAssetsPath(pipelineStateCreationDesc.shaderModule.psShaderPath);

        wrl::ComPtr<ID3DBlob> vertexBlob{nullptr};
        ::D3DReadFileToBlob(vsPath.c_str(), &vertexBlob);

        wrl::ComPtr<ID3DBlob> pixelBlob{nullptr};
        ::D3DReadFileToBlob(psPath.c_str(), &pixelBlob);

        if (!vertexBlob.Get())
        {
            fatalError(std::format("Shader {} not found.", wStringToString(vsPath)));
        }

        if (!pixelBlob.Get())
        {
            fatalError(std::format("Shader {} not found.", wStringToString(psPath)));
        }

        // note(rtarun9) : Blending not used for now, but the code is setup if needed.
        // Set up blend state (as d3dx12 doesn't seem to provide a automatic way to create pipeline state with blending
        // enabled).
        const D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {
            .BlendEnable = FALSE,
            .LogicOpEnable = FALSE,
            .SrcBlend = D3D12_BLEND_SRC_ALPHA,
            .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D12_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D12_BLEND_ONE,
            .DestBlendAlpha = D3D12_BLEND_ZERO,
            .BlendOpAlpha = D3D12_BLEND_OP_ADD,
            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        };

        // Primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives.
        // Basically, it sets up the rasterizer for the given primitive type. The primitive type must match with the IA
        // Topology type.
        D3D12_BLEND_DESC blendDesc = {
            .AlphaToCoverageEnable = FALSE,
            .IndependentBlendEnable = FALSE,
        };

        for (const uint32_t i : std::views::iota(0u, pipelineStateCreationDesc.rtvCount))
        {
            blendDesc.RenderTarget[i] = renderTargetBlendDesc;
        }

        // Setup depth stencil state.
        const D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {
            .DepthEnable = pipelineStateCreationDesc.depthFormat == DXGI_FORMAT_UNKNOWN ? FALSE : TRUE,
            .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = pipelineStateCreationDesc.depthComparisonFunc,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = PipelineState::s_rootSignature.Get(),
            .VS = CD3DX12_SHADER_BYTECODE(vertexBlob.Get()),
            .PS = CD3DX12_SHADER_BYTECODE(pixelBlob.Get()),
            .BlendState = blendDesc,
            .SampleMask = UINT32_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = depthStencilDesc,
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = pipelineStateCreationDesc.rtvCount,
            .DSVFormat = pipelineStateCreationDesc.depthFormat,
            .SampleDesc{.Count = 1u, .Quality = 0u},
            .NodeMask = 0u,
        };

        // Modify rasterizer state if front face winding order is counter clock wise.
        if (pipelineStateCreationDesc.frontFaceWindingOrder == FrontFaceWindingOrder::CounterClockWise)
        {
            psoDesc.RasterizerState.FrontCounterClockwise = true;
        }

        psoDesc.RasterizerState.CullMode = pipelineStateCreationDesc.cullMode;

        // Set RTV formats.
        for (const uint32_t i : std::views::iota(0u, pipelineStateCreationDesc.rtvCount))
        {
            psoDesc.RTVFormats[i] = pipelineStateCreationDesc.rtvFormats[i];
        }

        throwIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateObject)));

        m_pipelineStateObject->SetName(pipelineStateCreationDesc.pipelineName.c_str());
    }

    PipelineState::PipelineState(ID3D12Device5* const device,
                                 const ComputePipelineStateCreationDesc& pipelineStateCreationDesc)
    {
        const auto csPath = core::ResourceManager::getAssetsPath(pipelineStateCreationDesc.csShaderPath);

        wrl::ComPtr<ID3DBlob> computeBlob{nullptr};
        ::D3DReadFileToBlob(csPath.c_str(), &computeBlob);

        if (!computeBlob.Get())
        {
            fatalError(std::format("Shader : {} not found.", wStringToString(csPath)));
        }

        // Primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives.
        // Basically, it sets up the rasterizer for the given primitive type. The primitive type must match with the IA
        // Topology type.
        const D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = PipelineState::s_rootSignature.Get(),
            .CS = CD3DX12_SHADER_BYTECODE(computeBlob.Get()),
            .NodeMask = 0u,
        };

        throwIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateObject)));

        m_pipelineStateObject->SetName(pipelineStateCreationDesc.pipelineName.c_str());
    }

    void PipelineState::createBindlessRootSignature(ID3D12Device* const device, const std::wstring_view shaderPath)
    {
        const auto path = core::ResourceManager::getAssetsPath(shaderPath);
        const auto shader = core::ResourceManager::compileShader(ShaderTypes::Vertex, path, true);

        if (!shader.rootSignatureBlob.Get())
        {
            fatalError(std::format("Shader : {} not found.", wStringToString(path)));
        }

        throwIfFailed(device->CreateRootSignature(0u, shader.rootSignatureBlob->GetBufferPointer(),
                                                  shader.rootSignatureBlob->GetBufferSize(),
                                                  IID_PPV_ARGS(&s_rootSignature)));
        s_rootSignature->SetName(L"Bindless Root Signature");
    }
} // namespace helios::gfx