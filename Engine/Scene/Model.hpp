#pragma once

#include "Pch.hpp"

#include "Graphics/API/Resources.hpp"
#include "Graphics/API/Device.hpp"

#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace helios::scene
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
		void Update()
		{
			DirectX::XMVECTOR scalingVector = DirectX::XMLoadFloat3(&data.scale);
			DirectX::XMVECTOR rotationVector = DirectX::XMLoadFloat3(&data.rotation);
			DirectX::XMVECTOR translationVector = DirectX::XMLoadFloat3(&data.translate);

			DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixScalingFromVector(scalingVector) * DirectX::XMMatrixRotationRollPitchYawFromVector(rotationVector) * DirectX::XMMatrixTranslationFromVector(translationVector);
			TransformBuffer updatedTransformBuffer
			{
				.modelMatrix = modelMatrix,
				.inverseModelMatrix = DirectX::XMMatrixInverse(nullptr, modelMatrix),
			};
			
			transformBuffer->Update(&updatedTransformBuffer);
		}

		Transform() = default;
		Transform(const Transform& other) : data(other.data)
		{
		}
	};

	// This struct stores the texture's required for a PBR material. If a texture does not exist, it will be null, in which case the index (used to index into descriptor heap) will be -1.
	// The shader will accordingly set a null view and not use that particular texture.
	struct PBRMaterial
	{
		std::shared_ptr<gfx::Texture> albedoTexture{};
		std::shared_ptr<gfx::Texture> normalTexture{};
		std::shared_ptr<gfx::Texture> metalRoughnessTexture{};
		std::shared_ptr<gfx::Texture> aoTexture{};
		std::shared_ptr<gfx::Texture> emissiveTexture{};
	};

	// Stores all data required by a mesh (necessary buffer's and material).
	struct Mesh
	{
		std::shared_ptr<gfx::Buffer> positionBuffer{};
		std::shared_ptr<gfx::Buffer> textureCoordsBuffer{};
		std::shared_ptr<gfx::Buffer> normalBuffer{};
		std::shared_ptr<gfx::Buffer> tangentBuffer{};
		std::shared_ptr<gfx::Buffer> indexBuffer{};
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
	// Most smart pointers are shared since multiple model's may have been created from the same path, so they refer / point to same texture / mesh etc.
	// note(rtarun9) : CURRENTLY THIS CLASS DOES NOT HANDLE CHECKING OF MODEL IS ALREADY LOADED : THERE SEEMS TO BE SOME PROBLEM WITH THE USE OF UNIQUE_PTR's.
	class Model
	{
	public:
		Model(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc);

		Transform& GetTransform() { return mTransform; };

		void UpdateTransformUI(const ui::UIManager* uiManager);

		void Render(const gfx::GraphicsContext* graphicsContext, const SceneRenderResources& sceneRenderResources);

	private:
		void LoadNode(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc, uint32_t nodeIndex, tinygltf::Model& model);
	
	protected:
		std::vector<Mesh> mMeshes{};
		
		Transform mTransform{};

		// Used to prevent unnecessary loading of already loaded models.
		std::wstring mModelPath{};
		std::wstring mModelName{};
		std::wstring mModelDirectory{};

		// Holds all the models loaded in (key : model path) format. If model has already been loaded, it need not go through loading process again.
		//static inline std::map<std::wstring, Model> sLoadedGLTFModels{};
	};
}