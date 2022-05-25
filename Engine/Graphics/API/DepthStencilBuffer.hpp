#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	// Depth stencil abstraction : currently only works for depth and not stencil buffer.
	class DepthStencilBuffer
	{
	public:
		void Init(ID3D12Device* const device, Descriptor& dsvDescriptor, Descriptor& srvDescriptr, DXGI_FORMAT format, uint32_t width, uint32_t height, std::wstring_view bufferName);

		uint32_t GetBufferIndex() const { return m_BufferIndexInDescriptorHeap; }
		uint32_t GetSRVIndex() const { return m_SRVIndexInDescriptorHeap; }

		ID3D12Resource* const GetResource() const { return m_DepthStencilBuffer.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer{};
		uint32_t m_BufferIndexInDescriptorHeap{};
		uint32_t m_SRVIndexInDescriptorHeap{};
	};
}


