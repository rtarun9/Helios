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

	// Struct for various resource types : used to make resource creation simpler.
	// Will be updated as new requirements are found.
	struct ResourceCreationDesc
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		bool isCPUVisible{false};
	};

	struct SRVCreationDesc
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	};

}