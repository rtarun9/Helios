#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	// Depth stencil abstraction : currently only works for depth and not stencil buffer.
	class DepthStencilBuffer
	{
	public:
		DepthStencilBuffer(ID3D12Device5* const device, Descriptor* const dsvDescriptor, Descriptor* const srvDescriptor, DXGI_FORMAT format, uint32_t width, uint32_t height, std::wstring_view bufferName);

		uint32_t GetBufferIndex() const { return mBufferIndexInDescriptorHeap; }
		uint32_t GetSRVIndex() const { return mSRVIndexInDescriptorHeap; }

		ID3D12Resource* const GetResource() const { return mDepthStencilBuffer.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer{};
		uint32_t mBufferIndexInDescriptorHeap{};
		uint32_t mSRVIndexInDescriptorHeap{};
	};
}


