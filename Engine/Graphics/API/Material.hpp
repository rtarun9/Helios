#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	// Material : Abstraction over pipeline state and root signature (Graphics and Compute pipelines).
	// As the engine is bindless, there is a common state root signature used for both Graphics and Compute pipeline's.
	struct Material
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState{};

		// Root Signature is made static since the renderer is entirely bindless, and a single RootSignature is sufficient for all shaders.
		static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature{};
		
		static void CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view shaderPath);

		void Bind(ID3D12GraphicsCommandList* const commandList) const;
		void BindCS(ID3D12GraphicsCommandList* const commandList) const;

		static void BindRootSignature(ID3D12GraphicsCommandList* const commandList);
		static void BindRootSignatureCS(ID3D12GraphicsCommandList* const commandList);

		void BindPSO(ID3D12GraphicsCommandList* const commandList) const;

		static D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const vertexShaderBlob, ID3DBlob* const pixelShaderBlob, uint32_t rtvCount = 1u, DXGI_FORMAT rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT, bool depthEnable = true, bool cubeMap = false);
		static D3D12_COMPUTE_PIPELINE_STATE_DESC CreateComputePSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const computeShaderBlob);

		// Creates graphics pipeline state object and returns a Material.
		static Material CreateMaterial(ID3D12Device* const device, std::wstring_view vsShaderPath, std::wstring_view psShaderPath, std::wstring_view materialName, uint32_t rtvCount = 1u, DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT, bool isCubeMap = false);

		// Creates compute pipeline state object and returns a Material.
		static Material CreateMaterial(ID3D12Device* const device, std::wstring_view csShaderPath, std::wstring_view materialName);
	};
}


