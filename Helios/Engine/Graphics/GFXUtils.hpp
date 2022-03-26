#pragma once

#include "Pch.hpp"

namespace helios::gfx::utils
{
	inline void TransitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource,
		D3D12_RESOURCE_STATES previouState, D3D12_RESOURCE_STATES newState)
	{
		CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, previouState, newState);
		commandList->ResourceBarrier(1u, &transitionBarrier);
	}

	inline void ClearRTV(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, std::span<const float> clearColor)
	{
		commandList->ClearRenderTargetView(rtv, clearColor.data(), 0u, nullptr);
	}

	inline void ClearDepthBuffer(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depthClearValue = 1.0f)
	{
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depthClearValue, 1u, 0u, nullptr);
	}

	// Create a GPU buffer that returns two buffers : The final Destination buffer and an intermediate buffer (that is used to transfer data from the CPU to the GPU).
	// The intermediate buffer needs to be in scope until the command's in the list have finished execution, hence why is it being returned along with destination buffer.
	template <typename T>
	inline std::pair<ID3D12Resource*, ID3D12Resource*> CreateGPUBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::span<const T> bufferData, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE)
	{
		if (bufferData.empty())
		{
			throw std::exception("Cannot create GPU Buffer for data of 0 bytes.");
		}

		ID3D12Resource* destinationResource{ nullptr };
		ID3D12Resource* intermediateResource{ nullptr };

		size_t bufferSize = bufferData.size_bytes();

		// Commited resource that acts as the destination resource.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperites(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resourceFlags);
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperites, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&destinationResource)));

		// Intermediate upload heap that is needed to transfer CPU buffer data into the GPU memory.
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC intermediateResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &intermediateResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource)));

		// Logic to transfer data from CPU to GPU.

		// TODo : Possible UB, since bufferData is not in scope while the commadn list is being executed, need to check this UB soon.
		D3D12_SUBRESOURCE_DATA subresourceData
		{
			.pData = bufferData.data(),
			.RowPitch = static_cast<long long>(bufferSize),
			.SlicePitch = static_cast<long long>(bufferSize)
		};

		UpdateSubresources(commandList, destinationResource, intermediateResource, 0u, 0u, 1u, &subresourceData);

		return { destinationResource, intermediateResource};
	}

	// Note : Following D3D12 helper functions are temporary.
	
	// Note : using std::wstring_view seems to give error's for some reason.
	inline D3D12_INPUT_ELEMENT_DESC CreateInputLayoutDesc(std::string_view semanticName, DXGI_FORMAT format)
	{
		auto semanticNameStr = semanticName.data();

		return
		{
				.SemanticName = semanticNameStr,
				.SemanticIndex = 0,
				.Format = format,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
		};
	}

	inline D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPSODesc(ID3D12RootSignature* rootSignatureBlob, ID3DBlob* vertexShaderBlob, ID3DBlob* pixelShaderBlob, DXGI_FORMAT rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT)
	{
		return D3D12_GRAPHICS_PIPELINE_STATE_DESC 
		{
			.pRootSignature = rootSignatureBlob,
			.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob),
			.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob),
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = UINT_MAX,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState
			{
				.DepthEnable = TRUE,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
				.StencilEnable = FALSE
			},
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
}