#include "Pch.hpp"

#include "PipelineState.hpp"

namespace helios::gfx
{
	PipelineState::PipelineState(ID3D12Device* const device, const GraphicsPipelineStateCreationDesc& pipelineStateCreationDesc)
	{
		wrl::ComPtr<ID3DBlob> vertexBlob;
		::D3DReadFileToBlob(pipelineStateCreationDesc.shaderModule.vsShaderPath.data(), &vertexBlob);

		wrl::ComPtr<ID3DBlob> pixelBlob;
		::D3DReadFileToBlob(pipelineStateCreationDesc.shaderModule.psShaderPath.data(), &pixelBlob);

		if (!vertexBlob.Get())
		{
			ErrorMessage(pipelineStateCreationDesc.shaderModule.vsShaderPath.data() + std::wstring(L" Not found"));
		}

		if (!pixelBlob.Get())
		{
			ErrorMessage(pipelineStateCreationDesc.shaderModule.psShaderPath.data() + std::wstring(L" Not found"));
		}

		// note(rtarun9) : Blending not used for now, but the code is setup if needed.
		// Set up blend state (as d3dx12 doesnt seem to provide a automatic way to create pipeline state with blending enabled).
		D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc
		{
			.BlendEnable = FALSE,
			.LogicOpEnable = FALSE,
			.SrcBlend = D3D12_BLEND_SRC_ALPHA,
			.DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
			.BlendOp = D3D12_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D12_BLEND_ONE,
			.DestBlendAlpha = D3D12_BLEND_ZERO,
			.BlendOpAlpha= D3D12_BLEND_OP_ADD,
			.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
		};

		// Primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives.
		// Basically, it sets up the rasterizer for the given primitive type. The primitive type must match with the IA Topology type.
		D3D12_BLEND_DESC blendDesc
		{
			.AlphaToCoverageEnable = TRUE,
			.IndependentBlendEnable = TRUE,
			.RenderTarget = renderTargetBlendDesc
		};

		// Setup depth stencil state.
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc
		{
			.DepthEnable = pipelineStateCreationDesc.depthFormat == DXGI_FORMAT_UNKNOWN ? FALSE : TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = pipelineStateCreationDesc.depthComparisonFunc,
			.StencilEnable = FALSE,
			.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = PipelineState::rootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(vertexBlob.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(pixelBlob.Get()),
			.BlendState = blendDesc,
			.SampleMask= UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = depthStencilDesc,
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

		// Modify rasterizer state if front face winding order is counter clock wise.
		if (pipelineStateCreationDesc.frontFaceWindingOrder == FrontFaceWindingOrder::CounterClockWise)
		{
			psoDesc.RasterizerState.FrontCounterClockwise = true;

		}


		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));

		pipelineStateObject->SetName(pipelineStateCreationDesc.pipelineName.c_str());
	}

	PipelineState::PipelineState(ID3D12Device* const device, const ComputePipelineStateCreationDesc& pipelineStateCreationDesc)
	{
		wrl::ComPtr<ID3DBlob> computeBlob;
		::D3DReadFileToBlob(pipelineStateCreationDesc.csShaderPath.data(), &computeBlob);

		if (!computeBlob.Get())
		{
			ErrorMessage(pipelineStateCreationDesc.csShaderPath.data() + std::wstring(L" Not found"));
		}

		// Primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives.
		// Basically, it sets up the rasterizer for the given primitive type. The primitive type must match with the IA Topology type.
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = PipelineState::rootSignature.Get(),
			.CS = CD3DX12_SHADER_BYTECODE(computeBlob.Get()),
			.NodeMask = 0u,
		};

		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));

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
