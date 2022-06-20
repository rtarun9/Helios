#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	class Device;

	// Wrapper class for CommandList used only for copying data from CPU to GPU only memory.
	// Provides of easy and simple functions to record commands for execution by GPU.
	// The command queue will contain a queue of command list, which can be passed into the UploadContext's constructor to create a UploadContext object.
	// note (rtarun9) : This design is subject to change.
	class UploadContext
	{
	public:
		UploadContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		ID3D12GraphicsCommandList* const GetCommandList() { return mCommandList.Get(); }

		// Core functionalities.
		

	private:
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	};
}


