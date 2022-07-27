#pragma once

/*
#include "Pch.hpp"

#include "Graphics/API/Resources.hpp"
#include "Graphics/API/Device.hpp"

#include "Common/ConstantBuffers.hlsli"

#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace helios
{
	// note(rtarun9) : Currently there is a bit of confusion between the name of structs involved in transformation (Transform, TransformComponent, and TransformBuffer).
	// In the future, a standard naming scheme must be introduced.
	struct TransformComponent
	{
		DirectX::XMFLOAT3 rotation{0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
		DirectX::XMFLOAT3 translate{0.0f, 0.0f, 0.0f};
	};

	struct Transform
	{
		TransformComponent data{};
		std::unique_ptr<gfx::Buffer> transformBuffer{};

		// note(rtarun9) : Making an exception here where view and projection matrix are not wrapped into a struct, for code simplicity.
		void Update(const gfx::GraphicsContext* graphicsContext, DirectX::XMMATRIX& viewMatrix, DirectX::XMMATRIX& projectionMatrix)
		{
			DirectX::XMVECTOR scalingVector = DirectX::XMLoadFloat3(&data.scale);
			DirectX::XMVECTOR rotationVector = DirectX::XMLoadFloat3(&data.rotation);
			DirectX::XMVECTOR translationVector = DirectX::XMLoadFloat3(&data.translate);

			DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixScalingFromVector(scalingVector) * DirectX::XMMatrixRotationRollPitchYawFromVector(rotationVector) * DirectX::XMMatrixTranslationFromVector(translationVector);
			TransformBuffer updatedTransformBuffer
			{
				.modelMatrix = modelMatrix,
				.inverseModelMatrix = DirectX::XMMatrixInverse(nullptr, modelMatrix),
				.viewMatrix = viewMatrix,
				.projectionMatrix = projectionMatrix
			};
			
			transformBuffer->Update(&updatedTransformBuffer);
		}
	};

	// This struct stores the indices of various textures (index is used to view / index into the descriptor heap, as the renderer using Bindless Rendering).
	// If a particular GLTF mesh does not have one of the textures, index is simply -1.
	struct PBRMaterial
	{
		uint32_t albedoTextureIndex{};
		uint32_t normalTextureIndex{};
		uint32_t metalRoughnessTextureIndex{};
		uint32_t aoTextureIndex{};
		uint32_t emissiveTextureIndex{};
	};

	// Stores all data required by a mesh (necessary buffer's and material).
	struct Mesh
	{
		gfx::Buffer positionBuffer{};
		gfx::Buffer textureCoordsBuffer{};
		gfx::Buffer normalBuffer{};
		gfx::Buffer tangentBuffer{};

		gfx::Buffer indexBuffer{};
		uint32_t indicesCount{};

		PBRMaterial pbrMaterial{};
	};

	struct ModelCreationDesc
	{
		std::wstring modelPath{};
		std::wstring modelName{};
	};

	// Model class uses tinygltf for loading GLTF models.
	// Currently, only GLTF model loading is supported. This is mostly because of the much faster load times of this mesh type compared to .obj, .fbx, etc.
	class Model
	{
	public:
		Model(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc);
		Model(const Model& model);

		std::vector<Mesh> GetMeshes() const { return mMeshes; }

		Transform& GetTransform() { return mTransform; };

		void UpdateTransformUI();

		template <typename T>
		void Draw(ID3D12GraphicsCommandList* const commandList, T& renderResources);

	private:
		void LoadNode(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc, uint32_t nodeIndex, tinygltf::Model& model);
	
	protected:
		std::vector<Mesh> mMeshes{};
		
		Transform mTransform{};

		// Used to prevent unnecessary loading of already loaded models.
		std::wstring mModelPath{};
		std::wstring mModelName{};

		// Holds all the models loaded in (key : model path) format. If model has already been loaded, it need not go through loading process again.
		static inline std::map<std::wstring, Model> sLoadedGLTFModels{};
	};
}

*/