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

	void Material::Bind(ID3D12GraphicsCommandList* const commandList) const
	{
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->SetPipelineState(pipelineState.Get());
	}

	void Material::BindCS(ID3D12GraphicsCommandList* const commandList) const
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC Material::CreateGraphicsPSODesc(ID3D12RootSignature* const rootSignatureBlob, ID3DBlob* const vertexShaderBlob, ID3DBlob* const pixelShaderBlob, DXGI_FORMAT rtvFormat, bool depthEnable, bool isCubeMap)
	{
		if (isCubeMap)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = 
			{
				.pRootSignature = rootSignatureBlob,
				.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob),
				.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob),
				.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
				.SampleMask = UINT_MAX,
				.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1u,
				.RTVFormats = rtvFormat,
				.DSVFormat = {DXGI_FORMAT_D32_FLOAT},
				.SampleDesc
				{
					.Count = 1u,
					.Quality = 0u
				},
				.NodeMask = 0u,
			};

			psoDesc.DepthStencilState =
			{
				.DepthEnable = depthEnable,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
				.StencilEnable = FALSE,
			};

			psoDesc.RasterizerState.FrontCounterClockwise = true;

			return psoDesc;
		}

		return D3D12_GRAPHICS_PIPELINE_STATE_DESC
		{
			.pRootSignature = rootSignatureBlob,
			.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob),
			.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1u,
			.RTVFormats = rtvFormat,
			.DSVFormat = {DXGI_FORMAT_D32_FLOAT},
			.SampleDesc
			{
				.Count = 1u,
				.Quality = 0u
			},
			.NodeMask = 0u,
		};
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

	Material Material::CreateMaterial(ID3D12Device* const device, std::wstring_view vsShaderPath, std::wstring_view psShaderPath, std::wstring_view materialName, DXGI_FORMAT format, bool isCubeMap)
	{
		wrl::ComPtr<ID3DBlob> vertexBlob;
		::D3DReadFileToBlob(vsShaderPath.data(), &vertexBlob);

		wrl::ComPtr<ID3DBlob> pixelBlob;
		::D3DReadFileToBlob(psShaderPath.data(), &pixelBlob);

		if (!vertexBlob.Get() || !pixelBlob.Get())
		{
			ErrorMessage(psShaderPath.data() + std::wstring(L" Not found"));
		}

		wrl::ComPtr<ID3D12PipelineState> pso;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreateGraphicsPSODesc(Material::rootSignature.Get(), vertexBlob.Get(), pixelBlob.Get(), format, true, isCubeMap);
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		auto psoName = materialName.data() + std::wstring(L" PSO");
		pso->SetName(psoName.c_str());

		return { pso };
	}

	Material Material::CreateMaterial(ID3D12Device* const device, std::wstring_view csShaderPath, std::wstring_view materialName)
	{
		wrl::ComPtr<ID3DBlob> computeBlob;
		::D3DReadFileToBlob(csShaderPath.data(), &computeBlob);

		if (!computeBlob.Get())
		{
			ErrorMessage(csShaderPath.data() + std::wstring(L" Not found"));
		}

		wrl::ComPtr<ID3D12PipelineState> pso;

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = CreateComputePSODesc(Material::rootSignature.Get(), computeBlob.Get());
		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		auto psoName = materialName.data() + std::wstring(L" PSO");
		pso->SetName(psoName.c_str());

		return { pso };
	}
}
