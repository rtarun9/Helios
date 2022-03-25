#pragma once

#include "Pch.hpp"

#include "GFXUtils.hpp"

namespace helios::gfx
{
	class StructuredBuffer
	{
	public:
		template <typename T>
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::span<const T> data, D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE)
		{
			auto gpuBuffer = gfx::utils::CreateGPUBuffer<T>(device, commandList, data, resourceFlag);
			m_DestinationResource = gpuBuffer.first;
			m_IntermediateResource = gpuBuffer.second;
		}

		ID3D12Resource* GetResource();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_DestinationResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_IntermediateResource;
	};
}


