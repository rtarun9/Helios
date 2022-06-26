#pragma once

#include "Pch.hpp"
/*
#include "StructuredBuffer.hpp"
#include "IndexBuffer.hpp"

#include "Descriptor.hpp"

// Main reference used : https://raw.githubusercontent.com/wiki/Microsoft/DirectXTK12/RenderTexture.h.
// TODO : Resizing.
namespace helios::gfx
{
	static constexpr std::array<DirectX::SimpleMath::Vector2, 4> RT_VERTEX_POSITIONS
	{
		DirectX::SimpleMath::Vector2(-1.0f, -1.0f),
		DirectX::SimpleMath::Vector2(-1.0f,   1.0f),
		DirectX::SimpleMath::Vector2(1.0f, 1.0f),
		DirectX::SimpleMath::Vector2(1.0f, -1.0f),
	};

	static constexpr std::array<DirectX::SimpleMath::Vector2, 4> RT_VERTEX_TEXTURE_COORDS
	{
		DirectX::SimpleMath::Vector2(0.0f, 1.0f),
		DirectX::SimpleMath::Vector2(0.0f, 0.0f),
		DirectX::SimpleMath::Vector2(1.0f, 0.0f),
		DirectX::SimpleMath::Vector2(1.0f, 1.0f),
	};

	static constexpr std::array<uint32_t, 6> RT_INDICES
	{
		0u, 1u, 2u,
		0u, 2u, 3u
	};

	// Abstraction for render target.
	// Contains static methods that will init common data (Position and Texture buffers). These static methods need to be called *before* calling other methods. This will be done by the device class, so the user (sand box application) need not do this.
	class RenderTarget
	{
	public:
		static constexpr uint32_t RT_INDICES_COUNT = 6u;

		RenderTarget(Device* const device, const RenderTargetDesc& renderTargetDesc, std::wstring_view rtvName);

		ID3D12Resource* const GetResource(uint32_t index = 0u) const { return mAllocations->resource.Get(); }

		uint32_t GetSRVIndex(uint32_t srvIndex = 0u) const { return mSRVIndexInDescriptorHeap; }
		uint32_t GetRTVIndex(uint32_t rtvIndex = 0u) const { return mRTVIndexInDescriptorHeap; };

		static uint32_t GetPositionBufferIndex() { return sPositionBuffer->GetSRVIndex(); };
		static uint32_t GetTextureCoordsBufferIndex() { return sTextureCoordsBuffer->GetSRVIndex(); };

		static void InitBuffers(Device* const device);

	public:
		static inline std::unique_ptr<IndexBuffer> sIndexBuffer{};

		static inline std::unique_ptr<StructuredBuffer<DirectX::SimpleMath::Vector2>> sPositionBuffer{};
		static inline std::unique_ptr<StructuredBuffer<DirectX::SimpleMath::Vector2>> sTextureCoordsBuffer{};

	private:
		std::unique_ptr<Allocation> mAllocations{};

		uint32_t mSRVIndexInDescriptorHeap{};
		uint32_t mRTVIndexInDescriptorHeap{};

		uint32_t mWidth{};
		uint32_t mHeight{};
	};
}
*/