#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	struct Material
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature{};
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState{};

		void Bind(ID3D12GraphicsCommandList* commandList)
		{
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(pipelineState.Get());
		}
	};
}


