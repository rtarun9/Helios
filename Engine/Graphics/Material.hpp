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
	};
}


