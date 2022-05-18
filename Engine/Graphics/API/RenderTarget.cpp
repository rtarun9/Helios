#include "Pch.hpp"

#include "RenderTarget.hpp"

namespace helios::gfx
{
	void RenderTarget::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, DXGI_FORMAT format, Descriptor& rtvDescriptor, Descriptor& srvDescriptor, uint32_t width, uint32_t height, uint32_t renderTargetCount, std::wstring_view rtvName)
	{
		// Create resource desc specifying that a RTV can be created for it, and it can transition into D3D12_RESOURCE_STATE_RENDER_TARGET.
		CD3DX12_RESOURCE_DESC rtvResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT64>(width), static_cast<UINT>(height), 1u, 1u, 1u, 0u, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_CLEAR_VALUE renderTargetClearValue
		{
			.Format = format,
			.Color = {0.0f, 0.0f, 0.0f, 1.0f},
		};
	
		m_Resources.resize(renderTargetCount);

		// Create Resource, RTV and SRV.
		for (uint32_t i = 0; i < renderTargetCount; ++i)
		{
			ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &rtvResourceDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &renderTargetClearValue, IID_PPV_ARGS(&m_Resources[i])));

			auto resourceName = rtvName.data() + std::to_wstring(i);
			m_Resources[i]->SetName(resourceName.c_str());

			device->CreateRenderTargetView(m_Resources[i].Get(), nullptr, rtvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
			device->CreateShaderResourceView(m_Resources[i].Get(), nullptr, srvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);


			m_RTVIndexInDescriptorHeap.push_back(rtvDescriptor.GetCurrentDescriptorIndex());
			m_SRVIndexInDescriptorHeap.push_back(srvDescriptor.GetCurrentDescriptorIndex());

			rtvDescriptor.OffsetCurrentHandle();
			srvDescriptor.OffsetCurrentHandle();
			
			gfx::utils::TransitionResource(commandList, m_Resources[i].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		m_Width = width;
		m_Height = height;

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