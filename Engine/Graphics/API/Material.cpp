#include "Pch.hpp"

#include "Material.hpp"

namespace helios::gfx
{
	void Material::CreateBindlessRootSignature(ID3D12Device* const device, std::wstring_view shaderPath) 
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

	void Material::BindGraphicsMaterial(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
	}

	void Material::BindComputeMaterial(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetComputeRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
	}

	void Material::BindRootSignature(ID3D12GraphicsCommandList* const commandList) 
	{
		commandList->SetGraphicsRootSignature(rootSignature.Get());
	}

	void Material::BindRootSignatureCS(ID3D12GraphicsCommandList* const commandList) 
	{
		commandList->SetComputeRootSignature(rootSignature.Get());
	}

	void Material::BindPSO(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetPipelineState(pipelineState.Get());
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC Material::CreateGraphicsPSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const vertexShaderBlob, ID3DBlob* const pixelShaderBlob, GraphicsMaterialData& graphicsMaterialData)
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
			.NumRenderTargets = graphicsMaterialData.rtvCount,
			.DSVFormat = {DXGI_FORMAT_D32_FLOAT},
			.SampleDesc
			{
				.Count = 1u,
				.Quality = 0u
			},
			.NodeMask = 0u,
		};

		// Mostly used for cubemaps, where depth func should be COMPARISON_FUNC_LESS_EQUAL.
		if (graphicsMaterialData.depthComparisonFunc != D3D12_COMPARISON_FUNC_LESS)
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

		for (uint32_t i = 0; i < graphicsMaterialData.rtvCount; ++i)
		{
			psoDesc.RTVFormats[i] = graphicsMaterialData.format;
		}

		for (uint32_t i = graphicsMaterialData.rtvCount; i < 8u; ++i)
		{
			psoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		}

		return psoDesc;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC Material::CreateComputePSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const computeShaderBlob)
	{
		return D3D12_COMPUTE_PIPELINE_STATE_DESC
		{
			.pRootSignature = rootSignatureBlob,
			.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob),
			.NodeMask = 0u,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
	}

	Material Material::CreateMaterial(ID3D12Device* const device, std::variant<GraphicsMaterialData, ComputeMaterialData> materialData, std::wstring_view materialName)
	{
		switch (materialData.index())
		{
			case EnumClassValue(MaterialTypes::GraphicsMaterialData):
			{
				GraphicsMaterialData graphicsMaterialData = std::get<GraphicsMaterialData>(materialData);
				
				wrl::ComPtr<ID3DBlob> vertexBlob;
				::D3DReadFileToBlob(graphicsMaterialData.vsShaderPath.data(), &vertexBlob);

				wrl::ComPtr<ID3DBlob> pixelBlob;
				::D3DReadFileToBlob(graphicsMaterialData.psShaderPath.data(), &pixelBlob);

				if (!vertexBlob.Get())
				{
					ErrorMessage(graphicsMaterialData.vsShaderPath.data() + std::wstring(L" Not found"));
				}

				if (!pixelBlob.Get())
				{
					ErrorMessage(graphicsMaterialData.psShaderPath.data() + std::wstring(L" Not found"));
				}

				wrl::ComPtr<ID3D12PipelineState> pso;

				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreateGraphicsPSODesc(Material::rootSignature.Get(), vertexBlob.Get(), pixelBlob.Get(), graphicsMaterialData);
				ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
				auto psoName = materialName.data() + std::wstring(L" PSO");
				pso->SetName(psoName.c_str());

				return { pso };
			}
			break;

			case EnumClassValue(MaterialTypes::ComputeMaterialData):
			{
				ComputeMaterialData computeMaterialData = std::get<ComputeMaterialData>(materialData);

				wrl::ComPtr<ID3DBlob> computeBlob;
				::D3DReadFileToBlob(computeMaterialData.csShaderPath.data(), &computeBlob);

				if (!computeBlob.Get())
				{
					ErrorMessage(computeMaterialData.csShaderPath.data() + std::wstring(L" Not found"));
				}

				wrl::ComPtr<ID3D12PipelineState> pso;

				D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = CreateComputePSODesc(Material::rootSignature.Get(), computeBlob.Get());
				ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
				auto psoName = materialName.data() + std::wstring(L" PSO");
				pso->SetName(psoName.c_str());

				return { pso };
			}
			break;
		}

		return {};
	}
}
