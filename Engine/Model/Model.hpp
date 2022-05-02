#pragma once

#include "Pch.hpp"

#include "Graphics/StructuredBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include "Graphics/ConstantBuffer.hpp"
#include "Graphics/Descriptor.hpp"
#include "Graphics/Texture.hpp"

#include "ConstantBuffers.hlsli"

namespace helios
{
	struct TransformComponent
	{
		DirectX::XMFLOAT3 rotation{0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
		DirectX::XMFLOAT3 translate{0.0f, 0.0f, 0.0f};
	};

	// Model class uses tinygltf for loading GLTF models.
	// If a non - pbr model is to be loaded, an optional 'textureIndex' can be specified, that is the texture index of the base color of the model. This will eventually be removed in the future when all materials are PBR.
	class Model
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::wstring_view modelPath, gfx::Descriptor& cbDescriptor, std::wstring_view modelName, uint32_t textureIndex = -1);

		ID3D12Resource* const GetPositionBuffer() const { return m_PositionBuffer.GetResource(); }
		ID3D12Resource* const GetTextureCoordsBuffer() const { return m_TextureCoordsBuffer.GetResource(); };
		ID3D12Resource* const GetNormalBuffer() const { return m_NormalBuffer.GetResource(); };
		ID3D12Resource* const GetTangetBuffer() const { return m_TangentBuffer.GetResource(); };
		ID3D12Resource* const GetTransformBuffer() const { return m_TransformConstantBuffer.GetResource(); }

		gfx::StructuredBuffer<DirectX::XMFLOAT3> GetPositionStructuredBuffer() const { return m_PositionBuffer; };
		gfx::StructuredBuffer<DirectX::XMFLOAT2> GetTextureStructuredBuffer() const { return m_TextureCoordsBuffer; };
		gfx::StructuredBuffer<DirectX::XMFLOAT3> GetNormalStructuredBuffer() const { return m_NormalBuffer; };
		gfx::StructuredBuffer<DirectX::XMFLOAT4> GetTangentStructuredBuffer() const { return m_TangentBuffer; };
		gfx::IndexBuffer GetIndexBuffer() const { return m_IndexBuffer; };

		uint32_t GetPositionBufferIndex() const { return m_PositionBuffer.GetSRVIndex(); };
		uint32_t GetTextureCoordsBufferIndex() const { return m_TextureCoordsBuffer.GetSRVIndex(); };
		uint32_t GetNormalBufferIndex() const { return m_NormalBuffer.GetSRVIndex(); };
		uint32_t GetTangentBufferIndex() const { return m_TangentBuffer.GetSRVIndex(); };

		uint32_t GetIndicesCount() const { return m_IndicesCount; }

		uint32_t GetTransformCBufferIndex() const { return m_TransformCBufferIndexInDescriptorHeap; }

		uint32_t GetTextureIndex() const { return m_TextureIndex; };

		TransformComponent& GetTransform() { return m_TransformData; };

		void UpdateData();
		void UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix);

		void Draw(ID3D12GraphicsCommandList* const commandList);

	private:
		gfx::StructuredBuffer<DirectX::XMFLOAT3> m_PositionBuffer{};
		gfx::StructuredBuffer<DirectX::XMFLOAT2> m_TextureCoordsBuffer{};
		gfx::StructuredBuffer<DirectX::XMFLOAT3> m_NormalBuffer{};
		gfx::StructuredBuffer<DirectX::XMFLOAT4> m_TangentBuffer{};

		gfx::IndexBuffer m_IndexBuffer{};
		gfx::ConstantBuffer<TransformData> m_TransformConstantBuffer{};

		uint32_t m_IndicesCount{};

		TransformComponent m_TransformData{};

		// Note : Currently only used for non - PBR Models.
		uint32_t m_TextureIndex{};

		uint32_t m_TransformCBufferIndexInDescriptorHeap{};

		std::wstring m_ModelName{};

		// Holds all the models loaded in (key : model path). If model has already been loaded, it need not go through loading process again.
		static inline std::map<std::wstring, Model> s_LoadedGLTFModels{};
	};
}

