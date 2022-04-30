#include "Pch.hpp"

#include "RenderTarget.hpp"

namespace helios::gfx
{
	void RenderTarget::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, DXGI_FORMAT format, Descriptor& rtvDescriptor, Descriptor& srvDescriptor, uint32_t width, uint32_t height, std::wstring_view rtvName)
	{
		// Create resource desc specifying that a RTV can be created for it, and it can transition into D3D12_RESOURCE_STATE_RENDER_TARGET.
		CD3DX12_RESOURCE_DESC rtvResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT64>(width), static_cast<UINT>(height), 1u, 1u, 1u, 0u, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_CLEAR_VALUE renderTargetClearValue
		{
			.Format = format,
			.Color = {0.0f, 0.0f, 0.0f, 1.0f},
		};

		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &rtvResourceDesc, 
			D3D12_RESOURCE_STATE_RENDER_TARGET, &renderTargetClearValue, IID_PPV_ARGS(&m_Resource)));

		m_Resource->SetName(rtvName.data());
	
		// Create RTV and SRV.
		device->CreateRenderTargetView(m_Resource.Get(), nullptr, rtvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
		device->CreateShaderResourceView(m_Resource.Get(), nullptr, srvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);

		m_RTVIndexInDescriptorHeap = rtvDescriptor.GetCurrentDescriptorIndex();
		m_SRVIndexInDescriptorHeap = srvDescriptor.GetCurrentDescriptorIndex();

		rtvDescriptor.OffsetCurrentHandle();
		srvDescriptor.OffsetCurrentHandle();

		m_Width = width;
		m_Height = height;

		gfx::utils::TransitionResource(commandList, m_Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void RenderTarget::Bind(ID3D12GraphicsCommandList* commandList)
	{
		auto rtIndexBufferView = RenderTarget::s_IndexBuffer.GetBufferView();

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetIndexBuffer(&rtIndexBufferView);
	}

	void RenderTarget::InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor)
	{
		s_IndexBuffer.Init(device, commandList, RT_INDICES, L"Render Target Index Buffer");

		s_PositionBuffer.Init(device, commandList, srvDescriptor, RT_VERTEX_POSITIONS, D3D12_RESOURCE_FLAG_NONE, L"Render Target Position Buffer");
		s_TextureCoordsBuffer.Init(device, commandList, srvDescriptor, RT_VERTEX_TEXTURE_COORDS, D3D12_RESOURCE_FLAG_NONE, L"Render Target Texture Coord Buffer");
	}
}