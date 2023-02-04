#pragma once

#include "Resources.hpp"

namespace helios::gfx
{
    // PipelineState : Abstraction over pipeline state and root signature (Graphics and Compute pipelines).
    // As the engine is bindless, there is a common state root signature used for both Graphics and Compute pipeline's. That is why root signature is made static.
    class PipelineState
    {
      public:
        // note(rtarun9) : Not defining any of the big 5 member functions (destructor, move assignment etc) since the
        // compiler generated onces should suffice (no deep copies required here).
        explicit PipelineState() = default;

        PipelineState(ID3D12Device5* const device, const GraphicsPipelineStateCreationDesc& pipelineStateCreationDesc);
        PipelineState(ID3D12Device5* const device, const ComputePipelineStateCreationDesc& pipelineStateCreationDesc);

        // The shader path passed in needs to be relative (with respect to root directory), it will internally find the complete path (with respect to the executable).
        static void createBindlessRootSignature(ID3D12Device* const device, const std::wstring_view shaderPath);

      public:
        wrl::ComPtr<ID3D12PipelineState> m_pipelineStateObject{};

        // Root Signature is made static since the renderer is entirely bindless, and a single RootSignature is
        // sufficient for all shaders.
        static inline wrl::ComPtr<ID3D12RootSignature> s_rootSignature{};
    };
} // namespace helios::gfx
