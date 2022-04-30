#pragma once

#include "Pch.hpp"

#include "IndexBuffer.hpp"

namespace helios::gfx
{
	void IndexBuffer::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::span<const uint32_t> data, std::wstring_view indexBufferName)
	{
		std::pair<ID3D12Resource*, ID3D12Resource*> indexBuffer = utils::CreateGPUBuffer<uint32_t>(device, commandList, data);
		m_Buffer = indexBuffer.first;
		m_IntermediateBuffer = indexBuffer.second;

		m_Buffer->SetName(indexBufferName.data());

		m_BufferView =
		{
			.BufferLocation = m_Buffer->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(data.size_bytes()),
			.Format = DXGI_FORMAT_R32_UINT
		};
	}
}