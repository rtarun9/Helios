#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	struct GraphicsMaterialData
	{
		std::wstring_view vsShaderPath{};
		std::wstring_view psShaderPath{};
		uint32_t rtvCount{1};
		DXGI_FORMAT format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
		D3D12_COMPARISON_FUNC depthComparisonFunc{D3D12_COMPARISON_FUNC_LESS};
	};

	struct ComputeMaterialData
	{
		std::wstring_view csShaderPath{};
	};
	
	enum class MaterialTypes : uint8_t
	{
		GraphicsMaterialData,
		ComputeMaterialData
	};

	// Material : Abstraction over pipeline state and root signature (Graphics and Compute pipelines).
	// As the engine is bindless, there is a common state root signature used for both Graphics and Compute pipeline's.
	struct Material
	{
	public:
		static void CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view shaderPath);

		// Bind the Graphics / Compute root signature and pso.
		void BindGraphicsMaterial(ID3D12GraphicsCommandList* const commandList) const;
		void BindComputeMaterial(ID3D12GraphicsCommandList* const commandList) const;

		static void BindRootSignature(ID3D12GraphicsCommandList* const commandList);
		static void BindRootSignatureCS(ID3D12GraphicsCommandList* const commandList);

		void BindPSO(ID3D12GraphicsCommandList* const commandList) const;

		// Creates graphics / compute pipeline state object and returns a Material.
		static Material CreateMaterial(ID3D12Device* const device, std::variant<GraphicsMaterialData, ComputeMaterialData> materialData, std::wstring_view materialName);


	private:
		static D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const vertexShaderBlob, ID3DBlob* const pixelShaderBlob, GraphicsMaterialData& graphicsMaterialData);
		static D3D12_COMPUTE_PIPELINE_STATE_DESC CreateComputePSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const computeShaderBlob);

	public:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState{};

		// Root Signature is made static since the renderer is entirely bindless, and a single RootSignature is sufficient for all shaders.
		static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature{};
	};
}


