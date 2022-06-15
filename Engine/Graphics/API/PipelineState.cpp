#include "Pch.hpp"

#include "PipelineState.hpp"

namespace helios::gfx
{
	PipelineState::PipelineState(ID3D12Device* const device, const GraphicsPipelineStateData& pipelineStateData, std::wstring_view pipelineStateName)
	{
		wrl::ComPtr<ID3DBlob> vertexBlob;
		::D3DReadFileToBlob(pipelineStateData.vsShaderPath.data(), &vertexBlob);

		wrl::ComPtr<ID3DBlob> pixelBlob;
		::D3DReadFileToBlob(pipelineStateData.psShaderPath.data(), &pixelBlob);

		if (!vertexBlob.Get())
		{
			ErrorMessage(pipelineStateData.vsShaderPath.data() + std::wstring(L" Not found"));
		}

		if (!pixelBlob.Get())
		{
			ErrorMessage(pipelineStateData.psShaderPath.data() + std::wstring(L" Not found"));
		}

		wrl::ComPtr<ID3D12PipelineState> pso;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreateGraphicsPSODesc(PipelineState::rootSignature.Get(), vertexBlob.Get(), pixelBlob.Get(), pipelineStateData);
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		auto psoName = pipelineStateName.data() + std::wstring(L" PSO");
		pso->SetName(psoName.c_str());
	}

	PipelineState::PipelineState(ID3D12Device* const device, const ComputePipelineStateData& pipelineStateData, std::wstring_view pipelineStateName)
	{
		wrl::ComPtr<ID3DBlob> computeBlob;
		::D3DReadFileToBlob(pipelineStateData.csShaderPath.data(), &computeBlob);
	
		if (!computeBlob.Get())
		{
			ErrorMessage(pipelineStateData.csShaderPath.data() + std::wstring(L" Not found"));
		}
	
		wrl::ComPtr<ID3D12PipelineState> pso;
	
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = CreateComputePSODesc(PipelineState::rootSignature.Get(), computeBlob.Get());
		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		auto psoName = pipelineStateName.data() + std::wstring(L" PSO");
		pso->SetName(psoName.c_str());
	
	}

	void PipelineState::CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view shaderPath) 
	{
		wrl::ComPtr<ID3DBlob> shaderBlob;
		::D3DReadFileToBlob(shaderPath.data(), &shaderBlob);

		if (!shaderBlob.Get())
		{
			ErrorMessage(shaderPath.data() + std::wstring(L" Not found"));
		}

		ThrowIfFailed(device->CreateRootSignature(0u, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
		auto rootSignatureName = std::wstring(L"Bindless Root Signature");
		rootSignature->SetName(rootSignatureName.c_str());
	}

	void PipelineState::BindGraphicsPipelineState(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
	}

	void PipelineState::BindComputePipelineState(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetComputeRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
	}

	void PipelineState::BindRootSignature(ID3D12GraphicsCommandList* const commandList)
	{
		commandList->SetGraphicsRootSignature(rootSignature.Get());
	}

	void PipelineState::BindRootSignatureCS(ID3D12GraphicsCommandList* const commandList)
	{
		commandList->SetComputeRootSignature(rootSignature.Get());
	}

	void PipelineState::BindPSO(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetPipelineState(pipelineState.Get());
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineState::CreateGraphicsPSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const vertexShaderBlob, ID3DBlob* const pixelShaderBlob, const GraphicsPipelineStateData& graphicsPipelineStateData)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = rootSignatureBlob,
			.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob),
			.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = graphicsPipelineStateData.rtvCount,
			.DSVFormat = {graphicsPipelineStateData.depthFormat},
			.SampleDesc
			{
				.Count = 1u,
				.Quality = 0u
			},
			.NodeMask = 0u,
		};

		// Mostly used for cubemaps, where depth func should be COMPARISON_FUNC_LESS_EQUAL.
		if (graphicsPipelineStateData.depthComparisonFunc != D3D12_COMPARISON_FUNC_LESS)
		{
			psoDesc.DepthStencilState =
			{
				.DepthEnable = true,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
				.StencilEnable = FALSE,
			};

			// Not entirely sure why this is needed only in this case when cube map is used. If not used here / outside the if block the entire scene is blank.
			psoDesc.RasterizerState.FrontCounterClockwise = true;
		}



		// Out of the 8 available RTV's, fill the rtv count of first 'rtvCount' rtv's to the desired format, and other to DXGI_FORMAT_UNKNOWN.

		for (uint32_t i = 0; i < graphicsPipelineStateData.rtvCount; ++i)
		{
			psoDesc.RTVFormats[i] = graphicsPipelineStateData.format;
		}

		for (uint32_t i = graphicsPipelineStateData.rtvCount; i < 8u; ++i)
		{
			psoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		}

		return psoDesc;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC PipelineState::CreateComputePSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const computeShaderBlob)
	{
		return D3D12_COMPUTE_PIPELINE_STATE_DESC
		{
			.pRootSignature = rootSignatureBlob,
			.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob),
			.NodeMask = 0u,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
	}
}
