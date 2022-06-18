#include "Pch.hpp"

#include "UploadContext.hpp"

namespace helios::gfx
{
	UploadContext::UploadContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, uint32_t alignment)
	{
		mCommandList = commandList;
	}

	void UploadContext::Upload()
	{

	}
}

