#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	class Device;

	// Wrapper class for Upload CommandList, which provides a set of easy and simple functions useful in creating GPU buffers.
	// The Device will have a UploadContext, unlike the GraphicsContext as it will be used in all CreateX functions.
	// note (rtarun9) : This design is subject to change. In the future I want to add a linear allocator, but for now this will do.
	class UploadContext
	{
	public:
		UploadContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, uint32_t alignment);

		ID3D12GraphicsCommandList* const GetCommandList() { return mCommandList.Get(); }
		ID3D12Resource* const GetResource() { return mUploadBuffer.Get(); }
		void* const GetCPUAddress() { return mCPUAddress; }

		// Core functionalities.
		void Upload();

	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
		Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer{};
		void* mCPUAddress{ nullptr };
	};
}


