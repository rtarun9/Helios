#pragma once

#include "Pch.hpp"

#include "Graphics/API/StructuredBuffer.hpp"
#include "Graphics/API/IndexBuffer.hpp"
#include "Graphics/API/ConstantBuffer.hpp"
#include "Graphics/API/Descriptor.hpp"
#include "Graphics/API/Texture.hpp"

#include "ConstantBuffers.hlsli"
#include "BindlessRS.hlsli"

#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace helios
{
	struct TransformComponent
	{
		DirectX::SimpleMath::Vector3 rotation{0.0f, 0.0f, 0.0f};
		DirectX::SimpleMath::Vector3 scale{1.0f, 1.0f, 1.0f};
		DirectX::SimpleMath::Vector3 translate{0.0f, 0.0f, 0.0f};
	};

	// Note : The PBRMaterial struct does not need to hold textures at all : It can be just indices.
	// However, since there is no 'resource mananger' abstraction yet, this is done (as the textures need to exist until command list execution is done).
	struct PBRMaterial
	{
		uint32_t albedoTextureIndex{};
		uint32_t normalTextureIndex{};
		uint32_t metalRoughnessTextureIndex{};
		uint32_t aoTextureIndex{};
		uint32_t emissiveTextureIndex{};
	};

	struct Mesh
	{
		gfx::StructuredBuffer<DirectX::SimpleMath::Vector3> positionBuffer{};
		gfx::StructuredBuffer<DirectX::SimpleMath::Vector2> textureCoordsBuffer{};
		gfx::StructuredBuffer<DirectX::SimpleMath::Vector3> normalBuffer{};
		gfx::StructuredBuffer<DirectX::SimpleMath::Vector4> tangentBuffer{};

		gfx::IndexBuffer indexBuffer{};
		uint32_t indicesCount{};

		PBRMaterial pbrMaterial{};
	};

	// Model class uses tinygltf for loading GLTF models.
	class Model
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::wstring_view modelPath, gfx::Descriptor& srvCbDescriptor, std::wstring_view modelName);

		std::vector<Mesh> GetMeshes() const { return m_Meshes; }

		uint32_t GetTransformCBufferIndex() const { return m_TransformCBufferIndexInDescriptorHeap; }

		TransformComponent& GetTransform() { return m_TransformData; };

		void UpdateData();
		void UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix);

		template <typename T>
		void Draw(ID3D12GraphicsCommandList* const commandList, T& renderResources);

	private:
		void LoadNode(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::string_view modelDirectoryPathStr, gfx::Descriptor& srvCbDescriptor, uint32_t nodeIndex, tinygltf::Model& model);
	
	protected:
		std::vector<Mesh> m_Meshes{};
		gfx::ConstantBuffer<TransformData> m_TransformConstantBuffer{};

		TransformComponent m_TransformData{};
		uint32_t m_TransformCBufferIndexInDescriptorHeap{};

		std::wstring m_ModelName{};

		// Holds all the models loaded in (key : model path). If model has already been loaded, it need not go through loading process again.
		static inline std::map<std::wstring, Model> s_LoadedGLTFModels{};
	};
}

