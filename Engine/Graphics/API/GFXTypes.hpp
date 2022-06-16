#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	// Struct with back buffer data.
	struct BackBuffer
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBufferResource;
		gfx::DescriptorHandle backBufferDescriptorHandle;

		ID3D12Resource* GetResource() { return backBufferResource.Get(); }
	};
}