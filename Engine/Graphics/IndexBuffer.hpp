#pragma once

#include "Pch.hpp"

#include "GFXUtils.hpp"

namespace helios::gfx
{
	class IndexBuffer
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::span<const uint32_t> data, std::wstring_view indexBufferName);

		D3D12_INDEX_BUFFER_VIEW GetBufferView() const;

		// The intermediate buffer is needed to be in scope until the command list has finished execution.
		// Because of this, the intermediate buffer is a data member.
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_IntermediateBuffer;

		D3D12_INDEX_BUFFER_VIEW m_BufferView{};
	};
}

