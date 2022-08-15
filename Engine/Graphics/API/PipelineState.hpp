#pragma once

#include "Pch.hpp"

#include "Resources.hpp"

namespace helios::gfx
{
	// PipelineState : Abstraction over pipeline state and root signature (Graphics and Compute pipelines).
	// As the engine is bindless, there is a common state root signature used for both Graphics and Compute pipeline's.
	class PipelineState
	{
	public:
		PipelineState() = default;
		PipelineState(ID3D12Device* const device, const GraphicsPipelineStateCreationDesc& pipelineStateCreationDesc);
		PipelineState(ID3D12Device* const device, const ComputePipelineStateCreationDesc& pipelineStateCreationDesc);

		static void CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view shaderPath);

	public:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateObject{};

		// Root Signature is made static since the renderer is entirely bindless, and a single RootSignature is sufficient for all shaders.
		static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature{};
	};
}


