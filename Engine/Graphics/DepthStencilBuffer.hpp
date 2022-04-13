#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	class DepthStencilBuffer
	{
	public:
		void Init(ID3D12Device* device, Descriptor& dsvDescriptor, uint32_t width, uint32_t height, std::wstring_view bufferName);

		uint32_t GetBufferIndex() const;

		ID3D12Resource* GetResource() const;

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer{};
		uint32_t m_BufferIndexInDescriptorHeap{};
	};
}


