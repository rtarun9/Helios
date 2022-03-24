#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"

// Main reference used : https://raw.githubusercontent.com/wiki/Microsoft/DirectXTK12/RenderTexture.h.
// TODO : Resizing.
namespace helios::gfx
{
	struct RTVertex
	{
		DirectX::XMFLOAT2 position{};
		DirectX::XMFLOAT2 textureCoord{};
	};

	// Will be moved soon.
	static constexpr std::array<RTVertex, 4> RT_VERTICES
	{
		RTVertex{.position = DirectX::XMFLOAT2(-1.0f, -1.0f),		.textureCoord = DirectX::XMFLOAT2(0.0f, 1.0f) },
		RTVertex{.position = DirectX::XMFLOAT2(-1.0f,   1.0f),		.textureCoord = DirectX::XMFLOAT2(0.0f, 0.0f) },
		RTVertex{.position = DirectX::XMFLOAT2(1.0f, 1.0f),			.textureCoord = DirectX::XMFLOAT2(1.0f, 0.0f) },
		RTVertex{.position = DirectX::XMFLOAT2(1.0f, -1.0f),		.textureCoord = DirectX::XMFLOAT2(1.0f, 1.0f) },
	};

	static constexpr std::array<uint32_t, 6> RT_INDICES
	{
		0, 1, 2,
		0, 2, 3
	};

	class RenderTarget
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXGI_FORMAT format, Descriptor& rtvDescriptor, Descriptor& srvDescriptor, uint32_t width, uint32_t height, std::wstring_view rtvName);
		

		ID3D12Resource* GetResource();

		D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle();
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle();

		D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle();

		static void InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
		static void Bind(ID3D12GraphicsCommandList* commandList);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;

		D3D12_CPU_DESCRIPTOR_HANDLE m_RTVCPUDescriptorHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_RTVGPUDescriptorHandle{};

		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVCPUDescriptorHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_SRVGPUDescriptorHandle{};

		uint32_t m_Width{};
		uint32_t m_Height{};

		static inline VertexBuffer s_VertexBuffer{};
		static inline IndexBuffer s_IndexBuffer{};
	};
}