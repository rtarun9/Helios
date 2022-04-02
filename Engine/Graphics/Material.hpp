#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	// Bindless setup (WIP)
	struct alignas(256) RenderResource
	{
		uint32_t positionBufferIndex{};
		uint32_t textureBufferIndex{};
		uint32_t textureIndex{};
	};

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


