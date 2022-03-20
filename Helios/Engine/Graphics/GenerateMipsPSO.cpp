#include "Pch.hpp"

#include "GenerateMipsPSO.hpp"

namespace helios::gfx
{
	void GenerateMipsPSO::Init(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE uavHandle)
	{
		// The root signature is defined in the shader itself.
		wrl::ComPtr<ID3DBlob> rootSignatureShaderBlob;

		D3DReadFileToBlob(L"Shaders/GenerateMipsCS.cso", &rootSignatureShaderBlob);
		ThrowIfFailed(device->CreateRootSignature(0u, rootSignatureShaderBlob->GetBufferPointer(), rootSignatureShaderBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));


		// Create the PSO.
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc
		{
			.pRootSignature = m_RootSignature.Get(),
			.CS = CD3DX12_SHADER_BYTECODE(rootSignatureShaderBlob.Get()),
			.NodeMask = 0u,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));

		uint32_t uavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (uint32_t i = 0; i < 4; i++)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D
				{
					.MipSlice = i,
					.PlaneSlice = 0u
				}
			};

			device->CreateUnorderedAccessView(nullptr, nullptr, &uavViewDesc, uavHandle);
			uavHandle.ptr += uavDescriptorSize;
		}
	}
}
