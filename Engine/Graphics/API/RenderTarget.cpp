#include "Pch.hpp"

#include "RenderTarget.hpp"
#include "Device.hpp"

namespace helios::gfx
{
	RenderTarget::RenderTarget(Device* const device, const RenderTargetDesc& renderTargetDesc, std::wstring_view rtvName)
	{
		ResourceCreationDesc resourceCreationDesc
		{
			.resourceDesc
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.Width = renderTargetDesc.width,
				.Height = renderTargetDesc.height,
				.DepthOrArraySize = 1u,
				.Format = renderTargetDesc.format,
				.SampleDesc
				{
				.Count = 1u,
				.Quality = 0u
				},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
			},
			.isCPUVisible = false,
			.optimizedClearValue
			{
				.Format = renderTargetDesc.format,
				.Color = {0.0f, 0.0f, 0.0f, 1.0f},
			}
		};
	

		// Create Resource, RTV and SRV.
		mAllocations = device->GetMemoryAllocator()->CreateResource(resourceCreationDesc, rtvName);

		// Default SRV and RTV creation descs.
		SRVCreationDesc srvCreationDesc
		{
			.srvDesc{}
		};

		RTVCreationDesc rtvCreationDesc
		{
			.rtvDesc{}
		};

		mSRVIndexInDescriptorHeap = device->CreateSRV(srvCreationDesc, mAllocations->resource.Get());
		mRTVIndexInDescriptorHeap = device->CreateRTV(rtvCreationDesc, mAllocations->resource.Get());
			
		mWidth = renderTargetDesc.width;
		mHeight = renderTargetDesc.height;
	}


	void RenderTarget::InitBuffers(Device* const device)
	{
		sIndexBuffer = std::make_unique<IndexBuffer>(device, RT_INDICES, L"Render Target Index Buffer");

		sPositionBuffer = std::make_unique<StructuredBuffer<DirectX::SimpleMath::Vector2>>(device, RT_VERTEX_POSITIONS, "Render Target Position Buffer");
		sTextureCoordsBuffer = std::make_unique<StructuredBuffer<DirectX::SimpleMath::Vector2>>(device, RT_VERTEX_TEXTURE_COORDS, L"Render Target Texture Coord Buffer");
	}
}