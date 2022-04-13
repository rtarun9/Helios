#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	struct Material
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState{};

		// Root Signature is made static since the renderer is entirely bindless, and a single RootSignature is sufficient for all shaders.
		static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature{};
		
		static void CreateBindlessRootSignature(ID3D12Device* device, std::wstring_view shaderPath);

		void Bind(ID3D12GraphicsCommandList* commandList) const
		{
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(pipelineState.Get());
		}

		void BindCS(ID3D12GraphicsCommandList* commandList) const
		{
			commandList->SetComputeRootSignature(rootSignature.Get());
			commandList->SetPipelineState(pipelineState.Get());
		}

		// Last param temporary.
		static D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPSODesc(ID3D12RootSignature* rootSignatureBlob, ID3DBlob* vertexShaderBlob, ID3DBlob* pixelShaderBlob, DXGI_FORMAT rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT, bool depthEnable = true, bool cubeMap = false);
		static D3D12_COMPUTE_PIPELINE_STATE_DESC CreateComputePSODesc(ID3D12RootSignature* rootSignatureBlob, ID3DBlob* computeShaderBlob);

		// Creates graphics pipeline state object and returns a Material.
		static Material CreateMaterial(ID3D12Device* device, std::wstring_view vsShaderPath, std::wstring_view psShaderPath, std::wstring_view materialName, DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT, bool isCubeMap = false);

		// Creates compute pipeline state object and returns a Material.
		static Material CreateMaterial(ID3D12Device* device, std::wstring_view csShaderPath, std::wstring_view materialName);
	};
}


