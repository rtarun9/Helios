#pragma once

#include "Pch.hpp"

#include "Include/Graphics/IndexBuffer.hpp"

namespace helios::gfx
{
	void IndexBuffer::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::span<const uint32_t> data)
	{
		auto indexBuffer = utils::CreateGPUBuffer<uint32_t>(device, commandList, data);
		m_Buffer = indexBuffer.first;
		m_IntermediateBuffer = indexBuffer.second;

		m_BufferView =
		{
			.BufferLocation = m_Buffer->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(data.size_bytes()),
			.Format = DXGI_FORMAT_R32_UINT
		};
	}

	D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetBufferView()
	{
		return m_BufferView;
	}
}