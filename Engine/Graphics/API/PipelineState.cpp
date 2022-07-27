#include "Pch.hpp"

#include "PipelineState.hpp"

namespace helios::gfx
{
	PipelineState::PipelineState(ID3D12Device* const device, const GraphicsPipelineStateCreationDesc& pipelineStateCreationDesc)
	{
		wrl::ComPtr<ID3DBlob> vertexBlob;
		::D3DReadFileToBlob(pipelineStateCreationDesc.vsShaderPath.data(), &vertexBlob);

		wrl::ComPtr<ID3DBlob> pixelBlob;
		::D3DReadFileToBlob(pipelineStateCreationDesc.psShaderPath.data(), &pixelBlob);

		if (!vertexBlob.Get())
		{
			ErrorMessage(pipelineStateCreationDesc.vsShaderPath.data() + std::wstring(L" Not found"));
		}

		if (!pixelBlob.Get())
		{
			ErrorMessage(pipelineStateCreationDesc.psShaderPath.data() + std::wstring(L" Not found"));
		}


		// Primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives.
		// Basically, it sets up the rasterizer for the given primitive type. The primitive type must match with the IA Topology type.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = PipelineState::rootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(vertexBlob.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(pixelBlob.Get()),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask= UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1u,
			.RTVFormats= pipelineStateCreationDesc.rtvFormat,
			.DSVFormat = pipelineStateCreationDesc.depthFormat,
			.SampleDesc
			{
				.Count = 1u,
				.Quality = 0u
			},
			.NodeMask = 0u,
		};

		if (pipelineStateCreationDesc.depthFormat == DXGI_FORMAT_UNKNOWN)
		{
			psoDesc.DepthStencilState =
			{
				.DepthEnable = false
			};
		}
		else
		{
			if (pipelineStateCreationDesc.depthComparisonFunc != D3D12_COMPARISON_FUNC_LESS)
			{
				psoDesc.DepthStencilState =
				{
					.DepthEnable = true,
					.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
					.DepthFunc = pipelineStateCreationDesc.depthComparisonFunc,
					.StencilEnable = FALSE,
				};
			}
		}

		// Not entirely sure why this is needed only in this case when cube map is used. If not used here / outside the if block the entire scene is blank.
		//psoDesc.RasterizerState.FrontCounterClockwise = true;

		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));

		pipelineStateObject->SetName(pipelineStateCreationDesc.pipelineName.c_str());
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
}
