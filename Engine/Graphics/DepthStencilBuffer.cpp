#include "Pch.hpp"

#include "DepthStencilBuffer.hpp"

namespace helios::gfx
{
	void DepthStencilBuffer::Init(ID3D12Device* const device, Descriptor& dsvDescriptor, uint32_t width, uint32_t height, std::wstring_view bufferName)
	{
		D3D12_CLEAR_VALUE clearValue
		{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.DepthStencil
			{
				.Depth = 1.0f,
				.Stencil = 0u
			}
		};

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC depthTextureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1u, 0u, 1u, 0u, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &depthTextureResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&m_DepthStencilBuffer)));

		m_DepthStencilBuffer->SetName(bufferName.data());

		// Create DSV for the texture
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
		{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE,
			.Texture2D
			{
				.MipSlice = 0u
			},
		};

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle;

		device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvDesc, dsvHandle);

		m_BufferIndexInDescriptorHeap = dsvDescriptor.GetCurrentDescriptorIndex();
		dsvDescriptor.OffsetCurrentHandle();
	}
}
