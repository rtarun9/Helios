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
		0u, 1u, 2u,
		0u, 2u, 3u
	};

	// Abstraction for render target.
	// Contains static methods that will init common data (Position and Texture buffers). These static methods need to be called *before* calling other methods.
	class RenderTarget
	{
	public:
		static constexpr uint32_t RT_INDICES_COUNT = 6u;

		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, DXGI_FORMAT format, Descriptor& rtvDescriptor, Descriptor& srvDescriptor, uint32_t width, uint32_t height, uint32_t renderTargetCount, std::wstring_view rtvName);

		ID3D12Resource* const GetResource(uint32_t index = 0u) const { return m_Resources.at(index).Get(); }

		uint32_t GetSRVIndex(uint32_t srvIndex = 0u) const { return m_SRVIndexInDescriptorHeap.at(srvIndex); }
		uint32_t GetRTVIndex(uint32_t rtvIndex = 0u) const { return m_RTVIndexInDescriptorHeap.at(rtvIndex); };

		static uint32_t GetPositionBufferIndex() { return s_PositionBuffer.GetSRVIndex(); };
		static uint32_t GetTextureCoordsBufferIndex() { return s_TextureCoordsBuffer.GetSRVIndex(); };

		static void InitBuffers(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, Descriptor& srvDescriptor);
		static void Bind(ID3D12GraphicsCommandList* const commandList);

	private:
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_Resources;

		std::vector<uint32_t> m_SRVIndexInDescriptorHeap{};
		std::vector<uint32_t> m_RTVIndexInDescriptorHeap{};

		uint32_t m_Width{};
		uint32_t m_Height{};

		static inline IndexBuffer s_IndexBuffer{};

		static inline StructuredBuffer<DirectX::XMFLOAT2> s_PositionBuffer{};
		static inline StructuredBuffer<DirectX::XMFLOAT2> s_TextureCoordsBuffer{};
	};
}