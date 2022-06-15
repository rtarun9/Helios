#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	struct GraphicsPipelineStateData
	{
		std::wstring_view vsShaderPath{};
		std::wstring_view psShaderPath{};
		uint32_t rtvCount{1};
		DXGI_FORMAT format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
		DXGI_FORMAT depthFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT };
		D3D12_COMPARISON_FUNC depthComparisonFunc{D3D12_COMPARISON_FUNC_LESS};
	};

	struct ComputePipelineStateData
	{
		std::wstring_view csShaderPath{};
	};

	// PipelineState : Abstraction over pipeline state and root signature (Graphics and Compute pipelines).
	// As the engine is bindless, there is a common state root signature used for both Graphics and Compute pipeline's.
	struct PipelineState
	{
	public:
		PipelineState(ID3D12Device* const device, const GraphicsPipelineStateData& pipelineStateData, std::wstring_view pipelineStateName);
		PipelineState(ID3D12Device* const device, const ComputePipelineStateData& pipelineStateData, std::wstring_view pipelineStateName);

		static void CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view shaderPath);

		// Bind the Graphics / Compute root signature and pso.
		void BindGraphicsPipelineState(ID3D12GraphicsCommandList* const commandList) const;
		void BindComputePipelineState(ID3D12GraphicsCommandList* const commandList) const;

		static void BindRootSignature(ID3D12GraphicsCommandList* const commandList);
		static void BindRootSignatureCS(ID3D12GraphicsCommandList* const commandList);

		void BindPSO(ID3D12GraphicsCommandList* const commandList) const;

	private:
		static D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const vertexShaderBlob, ID3DBlob* const pixelShaderBlob, const GraphicsPipelineStateData& graphicsPipelineStateData);
		static D3D12_COMPUTE_PIPELINE_STATE_DESC CreateComputePSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const computeShaderBlob);

	public:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState{};

		// Root Signature is made static since the renderer is entirely bindless, and a single RootSignature is sufficient for all shaders.
		static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature{};
	};
}


