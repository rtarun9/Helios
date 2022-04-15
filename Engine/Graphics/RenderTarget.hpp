#pragma once

#include "Pch.hpp"

#include "StructuredBuffer.hpp"
#include "IndexBuffer.hpp"

#include "Descriptor.hpp"

// Main reference used : https://raw.githubusercontent.com/wiki/Microsoft/DirectXTK12/RenderTexture.h.
// TODO : Resizing.
namespace helios::gfx
{
	static constexpr std::array<DirectX::XMFLOAT2, 4> RT_VERTEX_POSITIONS
	{
		DirectX::XMFLOAT2(-1.0f, -1.0f),
		DirectX::XMFLOAT2(-1.0f,   1.0f),
		DirectX::XMFLOAT2(1.0f, 1.0f),
		DirectX::XMFLOAT2(1.0f, -1.0f),
	};

	static constexpr std::array<DirectX::XMFLOAT2, 4> RT_VERTEX_TEXTURE_COORDS
	{
		DirectX::XMFLOAT2(0.0f, 1.0f),
		DirectX::XMFLOAT2(0.0f, 0.0f),
		DirectX::XMFLOAT2(1.0f, 0.0f),
		DirectX::XMFLOAT2(1.0f, 1.0f),
	};

	static constexpr std::array<uint32_t, 6> RT_INDICES
	{
		0, 1, 2,
		0, 2, 3
	};

	class RenderTarget
	{
	public:
		static constexpr uint32_t RT_INDICES_COUNT = 6u;

		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXGI_FORMAT format, Descriptor& rtvDescriptor, Descriptor& srvDescriptor, uint32_t width, uint32_t height, std::wstring_view rtvName);

		ID3D12Resource* GetResource() const;

		uint32_t GetSRVIndex() const;
		uint32_t GetRTVIndex() const;

		static uint32_t GetPositionBufferIndex();
		static uint32_t GetTextureCoordsBufferIndex();

		static void InitBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor);
		static void Bind(ID3D12GraphicsCommandList* commandList);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;

		uint32_t m_SRVIndexInDescriptorHeap{};
		uint32_t m_RTVIndexInDescriptorHeap{};

		uint32_t m_Width{};
		uint32_t m_Height{};

		static inline IndexBuffer s_IndexBuffer{};

		static inline StructuredBuffer s_PositionBuffer{};
		static inline StructuredBuffer s_TextureCoordsBuffer{};
	};
}