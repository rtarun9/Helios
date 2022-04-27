#pragma once

#include "Pch.hpp"

#include "Graphics/StructuredBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include "Graphics/ConstantBuffer.hpp"
#include "Graphics/Descriptor.hpp"
#include "Graphics/Texture.hpp"

namespace helios
{
	struct alignas(256) Transform
	{
		DirectX::XMMATRIX modelMatrix{DirectX::XMMatrixIdentity()};
		DirectX::XMMATRIX inverseModelMatrix{ DirectX::XMMatrixIdentity() };
		DirectX::XMMATRIX projectionViewMatrix{ DirectX::XMMatrixIdentity() };
	};

	struct TransformComponent
	{
		DirectX::XMFLOAT3 rotation{DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)};
		DirectX::XMFLOAT3 scale{ DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f) };
		DirectX::XMFLOAT3 translate{ DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f) };
	};

	class Model
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath, gfx::Descriptor& cbDescriptor, uint32_t textureIndex = -1);

		// Will not be used currently, only for future use (if any).
		ID3D12Resource* GetPositionBuffer() const;
		ID3D12Resource* GetTextureCoordsBuffer() const;
		ID3D12Resource* GetNormalBuffer() const;
		ID3D12Resource* GetBiTangetBuffer() const;
		ID3D12Resource* GetTangetBuffer() const;

		uint32_t GetPositionBufferIndex() const;
		uint32_t GetTextureCoordsBufferIndex() const;
		uint32_t GetNormalBufferIndex() const;
		uint32_t GetBiTangentBufferIndex() const;
		uint32_t GetTangentBufferIndex() const;

		uint32_t GetTransformCBufferIndex() const;

		uint32_t GetTextureIndex() const;

		TransformComponent& GetTransform();
		void UpdateData(std::wstring_view objectName);

		void UpdateTransformData(ID3D12GraphicsCommandList* commandList, DirectX::XMMATRIX projectionViewMatrix);

		void Draw(ID3D12GraphicsCommandList* commandList);

	private:
		gfx::StructuredBuffer m_PositionBuffer{};
		gfx::StructuredBuffer m_TextureCoordsBuffer{};
		gfx::StructuredBuffer m_NormalBuffer{};
		gfx::StructuredBuffer m_BitangentBuffer{};
		gfx::StructuredBuffer m_TangentBuffer{};

		gfx::IndexBuffer m_IndexBuffer{};
		gfx::ConstantBuffer<Transform> m_TransformConstantBuffer{};

		uint32_t m_IndicesCount{};

		TransformComponent m_TransformData{};

		// Note : Currently only used for non - PBR Models.
		uint32_t m_TextureIndex{};

		uint32_t m_TransformCBufferIndexInDescriptorHeap{};
	};
}

