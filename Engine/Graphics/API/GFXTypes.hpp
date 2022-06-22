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
		uint32_t size{};
		bool isCPUVisible{false};
		D3D12_CLEAR_VALUE optimizedClearValue{};
	};

	struct SRVCreationDesc
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	};

	struct RTVCreationDesc
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	};

	struct UploadBufferDesc
	{
		uint32_t bufferSize{};
	};
	
	struct RenderTargetDesc
	{
		DXGI_FORMAT format;
		uint32_t width{};
		uint32_t height{};
	};
}