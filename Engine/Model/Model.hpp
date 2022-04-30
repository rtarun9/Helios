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

	// Model class uses tinygltf for loading GLTF models.
	// If a non - pbr model is to be loaded, an optional 'textureIndex' can be specified, that is the texture index of the base color of the model. This will eventually be removed in the future when all materials are PBR.
	class Model
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::wstring_view modelPath, gfx::Descriptor& cbDescriptor, std::wstring_view modelName, uint32_t textureIndex = -1);

		// Will not be used currently, only for future use (if any).
		ID3D12Resource* const GetPositionBuffer() const { return m_PositionBuffer.GetResource(); }
		ID3D12Resource* const GetTextureCoordsBuffer() const { return m_TextureCoordsBuffer.GetResource(); };
		ID3D12Resource* const GetNormalBuffer() const { return m_NormalBuffer.GetResource(); };
		ID3D12Resource* const GetTangetBuffer() const { return m_TangentBuffer.GetResource(); };

		uint32_t GetPositionBufferIndex() const { return m_PositionBuffer.GetSRVIndex(); };
		uint32_t GetTextureCoordsBufferIndex() const { return m_TextureCoordsBuffer.GetSRVIndex(); };
		uint32_t GetNormalBufferIndex() const { return m_NormalBuffer.GetSRVIndex(); };
		uint32_t GetTangentBufferIndex() const { return m_TangentBuffer.GetSRVIndex(); };

		uint32_t GetTransformCBufferIndex() const { return m_TransformCBufferIndexInDescriptorHeap; }

		uint32_t GetTextureIndex() const { return m_TextureIndex; };

		TransformComponent& GetTransform() { return m_TransformData; };

		void UpdateData();
		void UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionViewMatrix);

		void Draw(ID3D12GraphicsCommandList* const commandList);

	private:
		gfx::StructuredBuffer<DirectX::XMFLOAT3> m_PositionBuffer{};
		gfx::StructuredBuffer<DirectX::XMFLOAT2> m_TextureCoordsBuffer{};
		gfx::StructuredBuffer<DirectX::XMFLOAT3> m_NormalBuffer{};
		gfx::StructuredBuffer<DirectX::XMFLOAT4> m_TangentBuffer{};

		gfx::IndexBuffer m_IndexBuffer{};
		gfx::ConstantBuffer<Transform> m_TransformConstantBuffer{};

		uint32_t m_IndicesCount{};

		TransformComponent m_TransformData{};

		// Note : Currently only used for non - PBR Models.
		uint32_t m_TextureIndex{};

		uint32_t m_TransformCBufferIndexInDescriptorHeap{};

		std::wstring m_ModelName{};
	};
}

