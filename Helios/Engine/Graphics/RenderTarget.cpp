#include "Pch.hpp"

#include "RenderTarget.hpp"

namespace helios::gfx
{
	void RenderTarget::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXGI_FORMAT format, Descriptor& rtvDescriptor, Descriptor& srvDescriptor, uint32_t width, uint32_t height, std::wstring_view rtvName)
	{
		// Create resource desc specifying that a RTV can be created for it, and it can transition into D3D12_RESOURCE_STATE_RENDER_TARGET.
		CD3DX12_RESOURCE_DESC rtvResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT64>(width), static_cast<UINT>(height), 1u, 1u, 1u, 0u, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_CLEAR_VALUE renderTargetClearValue
		{
			.Format = format,
			.Color = {0.1f, 0.1f, 0.1f, 1.0f},
		};

		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &rtvResourceDesc, 
			D3D12_RESOURCE_STATE_RENDER_TARGET, &renderTargetClearValue, IID_PPV_ARGS(&m_Resource)));

		m_Resource->SetName(rtvName.data());
	
		// Create RTV and SRV.
		device->CreateRenderTargetView(m_Resource.Get(), nullptr, rtvDescriptor.GetCurrentCPUDescriptorHandle());
		device->CreateShaderResourceView(m_Resource.Get(), nullptr, srvDescriptor.GetCurrentCPUDescriptorHandle());

		m_RTVCPUDescriptorHandle = rtvDescriptor.GetCurrentCPUDescriptorHandle();
		m_RTVGPUDescriptorHandle = rtvDescriptor.GetCurrentGPUDescriptorHandle();

		m_SRVCPUDescriptorHandle = srvDescriptor.GetCurrentCPUDescriptorHandle();
		m_SRVGPUDescriptorHandle = srvDescriptor.GetCurrentGPUDescriptorHandle();

		rtvDescriptor.OffsetCurrentDescriptorHandles();
		srvDescriptor.OffsetCurrentDescriptorHandles();

		m_Width = width;
		m_Height = height;

		gfx::utils::TransitionResource(commandList, m_Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void RenderTarget::Bind(ID3D12GraphicsCommandList* commandList)
	{
		auto rtVertexBufferView = RenderTarget::s_VertexBuffer.GetBufferView();
		auto rtIndexBufferView = RenderTarget::s_IndexBuffer.GetBufferView();

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0u, 1u, &rtVertexBufferView);
		commandList->IASetIndexBuffer(&rtIndexBufferView);
	}

	ID3D12Resource* RenderTarget::GetResource()
	{
		return m_Resource.Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRTVCPUDescriptorHandle()
	{
		return m_RTVCPUDescriptorHandle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetSRVCPUDescriptorHandle()
	{
		return m_SRVCPUDescriptorHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderTarget::GetRTVGPUDescriptorHandle()
	{
		return m_RTVGPUDescriptorHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderTarget::GetSRVGPUDescriptorHandle()
	{
		return m_SRVGPUDescriptorHandle;
	}

	void RenderTarget::InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	{
		s_VertexBuffer.Init<RTVertex>(device, commandList, RT_VERTICES, L"Render Target Vertex Buffer");
		s_IndexBuffer.Init(device, commandList, RT_INDICES, L"Render Target Index Buffer");
	}
}