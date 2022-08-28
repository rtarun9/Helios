#pragma once

#include "Graphics/API/Resources.hpp"
#include "Graphics/API/Device.hpp"

#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

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
	};


	// This struct stores the texture's required for a PBR material. If a texture does not exist, it will be null, in which case the index (used to index into descriptor heap) will be 0.
	// The shader will accordingly set a null view and not use that particular texture.
	// Each texture (if it exist) will have a sampler index associated with it, so we can use SamplerDescriptorHeap to index into the heap directly. If no sampler, index defaults to 0.
	struct PBRMaterial
	{
		std::shared_ptr<gfx::Texture> albedoTexture{};
		uint32_t albedoTextureSamplerIndex{};
		
		std::shared_ptr<gfx::Texture> normalTexture{};
		uint32_t normalTextureSamplerIndex{};

		std::shared_ptr<gfx::Texture> metalRoughnessTexture{};
		uint32_t metalRoughnessTextureSamplerIndex{};

		std::shared_ptr<gfx::Texture> aoTexture{};
		uint32_t aoTextureSamplerIndex{};

		std::shared_ptr<gfx::Texture> emissiveTexture{};
		uint32_t emissiveTextureSamplerIndex{};
	};

	// Stores all data required by a mesh (necessary buffer's and material).
	struct Mesh
	{
		std::shared_ptr<gfx::Buffer> positionBuffer{};
		std::shared_ptr<gfx::Buffer> textureCoordsBuffer{};
		std::shared_ptr<gfx::Buffer> normalBuffer{};
		std::shared_ptr<gfx::Buffer> tangentBuffer{};
		std::shared_ptr<gfx::Buffer> biTangentBuffer{};
		std::shared_ptr<gfx::Buffer> indexBuffer{};
		uint32_t indicesCount{};

		uint32_t materialIndex{};
	};

	struct ModelCreationDesc
	{
		std::wstring modelPath{};
		std::wstring modelName{};
	};

	// Model class uses tinygltf for loading GLTF models.
	// Currently, only GLTF model loading is supported. This is mostly because of the much faster load times of this mesh type compared to .obj, .fbx, etc.
	// Most smart pointers are shared since multiple model's may have been created from the same path, so they refer / point to same texture / mesh etc.
	// note(rtarun9) : CURRENTLY THIS CLASS DOES NOT HANDLE CHECKING OF MODEL IS ALREADY LOADED : THERE SEEMS TO BE SOME PROBLEM WITH THE USE OF UNIQUE_PTR's IN INTERNAL MEMBER VARIABLES.
	class Model
	{
	public:
		Model() = default;
		Model(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc);

		Transform* GetTransform() { return &mTransform; };
		std::wstring GetName() const { return mModelName; }

		void Render(const gfx::GraphicsContext* graphicsContext, const SceneRenderResources& sceneRenderResources);
		void Render(const gfx::GraphicsContext* graphicsContext, LightRenderResources& lightRenderResources);
		void Render(const gfx::GraphicsContext* graphicsContext, SkyBoxRenderResources& skyBoxrenderResources);
		void Render(const gfx::GraphicsContext* graphicsContext, ShadowMappingRenderResources& shadowMappingRenderResources);

	private:
		void LoadNode(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc, uint32_t nodeIndex, tinygltf::Model& model);
		void LoadSamplers(const gfx::Device* device, tinygltf::Model& model);
		void LoadMaterials(const gfx::Device* device, tinygltf::Model& model);

		Transform mTransform{};
	
	public:
		std::wstring mModelName{};
	
	private:
		std::vector<Mesh> mMeshes{};
		std::vector<PBRMaterial> mMaterials{};
		std::vector<uint32_t> mSamplers{};
			
		std::wstring mModelPath{};
		std::wstring mModelDirectory{};
	};
}
