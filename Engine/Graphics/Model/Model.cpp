#include "Pch.hpp"
/*

#include "Core/UIManager.hpp"

#include "Core/Helpers.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Model.hpp"

using namespace DirectX;

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace DirectX;

namespace helios
{
	Model::Model(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc)
	{
		mModelPath = modelCreationDesc.modelPath;
		mModelName = modelCreationDesc.modelName;

		// Placed here as it will be used in both braches : either if model is already loaded or not.
		gfx::BufferCreationDesc transformBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = mModelName + L" Transform Buffer",
		};

		mTransform.transformBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<TransformBuffer>(transformBufferCreationDesc, std::span<TransformBuffer, 0u>{}));

		std::string modelPathStr = WstringToString(mModelPath);
		std::string modelDirectoryPathStr = modelPathStr.substr(0, modelPathStr.find_last_of(L'/') + 1);

		std::string warning{};
		std::string error{};

		tinygltf::TinyGLTF context{};

		tinygltf::Model model{};

		// If a model with same path has already been loaded previously.
		if (sLoadedGLTFModels.find(mModelPath) != sLoadedGLTFModels.end())
		{
			Model model = sLoadedGLTFModels[mModelPath];

			mMeshes = model.GetMeshes();

			return;
		}

		if (!context.LoadASCIIFromFile(&model, &error, &warning, modelPathStr))
		{
			if (!error.empty())
			{
				ErrorMessage(StringToWString(error));
			}

			if (!warning.empty())
			{
				ErrorMessage(StringToWString(warning));
			}
		}

		// Build meshes.
		tinygltf::Scene& scene = model.scenes[model.defaultScene];

		for (const int& nodeIndex : scene.nodes)
		{
			LoadNode(device, modelCreationDesc, nodeIndex, model);
		}

		sLoadedGLTFModels.insert(std::pair<std::wstring, Model>(mModelPath, *this));
	}
	
	// The transform buffer is created within Model's parameterized constructor, not in the copy constructor as it needs access to the gfx::Device object.
	Model::Model(const Model& model) : mModelName(model.mModelName), mModelPath(model.mModelPath), mMeshes(model.mMeshes)
	{
	}

	void Model::LoadNode(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc, uint32_t nodeIndex, tinygltf::Model& model)
	{
		tinygltf::Node& node = model.nodes[nodeIndex];
		if (node.mesh < 0)
		{
			node.mesh = 0;
		}

		tinygltf::Mesh& nodeMesh = model.meshes[node.mesh];
		for (size_t i = 0; i < nodeMesh.primitives.size(); ++i)
		{
			Mesh mesh{};

			const std::wstring meshNumber = std::to_wstring(i);
			const std::wstring meshName = mModelName + L" Mesh " + std::wstring(meshNumber.c_str());

			std::vector<math::XMFLOAT3> modelPositions{};
			std::vector<math::XMFLOAT2> modelTextureCoords{};
			std::vector<math::XMFLOAT3> modelNormals{};
			std::vector<math::XMFLOAT4> modelTangents{};

			std::vector<uint32_t> indices{};

			// Reference used : https://github.com/mateeeeeee/Adria-DX12/blob/fc98468095bf5688a186ca84d94990ccd2f459b0/Adria/Rendering/EntityLoader.cpp.

			// Get Accesor, buffer view and buffer for each attribute (position, textureCoord, normal).
			tinygltf::Primitive primitive = nodeMesh.primitives[i];
			tinygltf::Accessor& indexAccesor = model.accessors[primitive.indices];

			// Position data.
			tinygltf::Accessor& positionAccesor = model.accessors[primitive.attributes["POSITION"]];
			tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccesor.bufferView];
			tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];

			int positionByteStride = positionAccesor.ByteStride(positionBufferView);
			uint8_t const* const positions = &positionBuffer.data[positionBufferView.byteOffset + positionAccesor.byteOffset];

			// TextureCoord data.
			tinygltf::Accessor& textureCoordAccesor = model.accessors[primitive.attributes["TEXCOORD_0"]];
			tinygltf::BufferView& textureCoordBufferView = model.bufferViews[textureCoordAccesor.bufferView];
			tinygltf::Buffer& textureCoordBuffer = model.buffers[textureCoordBufferView.buffer];
			int textureCoordBufferStride = textureCoordAccesor.ByteStride(textureCoordBufferView);
			uint8_t const* const texcoords = &textureCoordBuffer.data[textureCoordBufferView.byteOffset + textureCoordAccesor.byteOffset];

			// Normal data.
			tinygltf::Accessor& normalAccesor = model.accessors[primitive.attributes["NORMAL"]];
			tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccesor.bufferView];
			tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
			int normalByteStride = normalAccesor.ByteStride(normalBufferView);
			uint8_t const* const normals = &normalBuffer.data[normalBufferView.byteOffset + normalAccesor.byteOffset];

			// Tangent data.
			tinygltf::Accessor& tangentAccesor = model.accessors[primitive.attributes["TANGENT"]];
			tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccesor.bufferView];
			tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
			int tangentByteStride = tangentAccesor.ByteStride(tangentBufferView);
			uint8_t  const* const tangents = &tangentBuffer.data[tangentBufferView.byteOffset + tangentAccesor.byteOffset];

			// Fill in the vertices array.
			for (size_t i : std::views::iota(0u, positionAccesor.count))
			{
				math::XMFLOAT3 position
				{
					 (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[0],
					 (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[1],
					 (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[2]
				};
	
	
				math::XMFLOAT2 textureCoord
				{
					(reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[0],
					1.0f - (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1],
				};

				math::XMFLOAT3 normal
				{
					(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0],
					(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1],
					(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2],
				};

				math::XMFLOAT4 tangent{};
				
				// Required as a model need not have tangents.
				if (tangentAccesor.bufferView)
				{
					tangent =
					{
						(reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[0],
						(reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[1],
						(reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[2],
						(reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[3],
					};
				}

				modelPositions.emplace_back(position);
				modelTextureCoords.emplace_back(textureCoord);
				modelNormals.emplace_back(normal);
				modelTangents.emplace_back(tangent);
			}

			// Get the index buffer data.
			tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccesor.bufferView];
			tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
			int indexByteStride = indexAccesor.ByteStride(indexBufferView);
			uint8_t const* const indexes = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccesor.byteOffset;

			// Fill indices array.
			for (size_t i : std::views::iota(0u, indexAccesor.count))
			{
				if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					indices.push_back(static_cast<uint32_t>((reinterpret_cast<uint16_t const*>(indexes + (i * indexByteStride)))[0]));
				}
				else if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					indices.push_back(static_cast<uint32_t>((reinterpret_cast<uint32_t const*>(indexes + (i * indexByteStride)))[0]));
				}
			}

			// Load material. (NOTE : Not complete implementation, but basic framework).
			tinygltf::Material gltfPBRMaterial = model.materials[primitive.material];

			gfx::Texture meshAlbedoTexture{};
			gfx::Texture meshNormalTexture{};
			gfx::Texture meshMetalRoughnessTexture{};
			gfx::Texture meshAoTexture{};
			gfx::Texture meshEmissiveTexture{};

			if (gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
			{
				tinygltf::Texture& albedoTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index];
				tinygltf::Image& albedoImage = model.images[albedoTexture.source];

				std::wstring albedoTexturePath = StringToWString(modelDirectoryPathStr.data() + albedoImage.uri);
				if (!albedoTexturePath.empty())
				{
					textureInitDeferredQueue.PushFunction([&, albedoTexturePath]() {meshAlbedoTexture.Init(device, commandList, srvCbDescriptor, gfx::NonHDRTextureData{ .texturePath = albedoTexturePath, .mipLevels = 1u, .isSRGB = true }, m_ModelName + L" Mesh" + meshNumber + L" Albedo Texture"); });
				}
			}

			if (gltfPBRMaterial.normalTexture.index >= 0)
			{
				tinygltf::Texture& normalTexture = model.textures[gltfPBRMaterial.normalTexture.index];
				tinygltf::Image& normalImage = model.images[normalTexture.source];

				std::wstring normalTexturePath = StringToWString(modelDirectoryPathStr.data() + normalImage.uri);
				if (!normalTexturePath.empty())
				{
					textureInitDeferredQueue.PushFunction([&, normalTexturePath]() {meshNormalTexture.Init(device, commandList, srvCbDescriptor, gfx::NonHDRTextureData{ .texturePath = normalTexturePath, .mipLevels = 1u, .isSRGB = false }, m_ModelName + L" Mesh" + meshNumber + L" Normal Texture"); });
				}
			}

			if (gltfPBRMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
			{
				tinygltf::Texture& metalRoughnessTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
				tinygltf::Image& metalRoughnessImage = model.images[metalRoughnessTexture.source];

				std::wstring metalRoughnessTexturePath = StringToWString(modelDirectoryPathStr.data() + metalRoughnessImage.uri);
				if (!metalRoughnessTexturePath.empty())
				{
					textureInitDeferredQueue.PushFunction([&, metalRoughnessTexturePath](){meshMetalRoughnessTexture.Init(device, commandList, srvCbDescriptor, gfx::NonHDRTextureData{ .texturePath = metalRoughnessTexturePath, .mipLevels = 1u, .isSRGB = false }, m_ModelName + L" Mesh" + meshNumber + L" Metal Roughness Texture");});
				}
			}

			if (gltfPBRMaterial.occlusionTexture.index >= 0)
			{
				tinygltf::Texture& aoTexture = model.textures[gltfPBRMaterial.occlusionTexture.index];
				tinygltf::Image& aoImage = model.images[aoTexture.source];

				std::wstring aoTexturePath = StringToWString(modelDirectoryPathStr.data() + aoImage.uri);
				if (!aoTexturePath.empty())
				{
					textureInitDeferredQueue.PushFunction([&, aoTexturePath]() {meshAoTexture.Init(device, commandList, srvCbDescriptor, gfx::NonHDRTextureData{ .texturePath = aoTexturePath, .mipLevels = 1u, .isSRGB = false }, m_ModelName + L" Mesh" + meshNumber + L" AO Texture"); });
				}
			}

			if (gltfPBRMaterial.emissiveTexture.index >= 0)
			{
				tinygltf::Texture& emissiveTexture = model.textures[gltfPBRMaterial.emissiveTexture.index];
				tinygltf::Image& emissiveImage = model.images[emissiveTexture.source];

				std::wstring emissiveTexturePath = StringToWString(modelDirectoryPathStr.data() + emissiveImage.uri);
				if (!emissiveTexturePath.empty())
				{
					textureInitDeferredQueue.PushFunction([&, emissiveTexturePath]() {meshEmissiveTexture.Init(device, commandList, srvCbDescriptor, gfx::NonHDRTextureData{ .texturePath = emissiveTexturePath, .mipLevels = 1u, .isSRGB = true }, m_ModelName + L" Mesh" + meshNumber + L" Emissive Texture"); });
				}
			}

			textureInitDeferredQueue.Execute();

			mesh.pbrMaterial.albedoTextureIndex = meshAlbedoTexture.GetTextureIndex();
			mesh.pbrMaterial.normalTextureIndex = meshNormalTexture.GetTextureIndex();
			mesh.pbrMaterial.metalRoughnessTextureIndex = meshMetalRoughnessTexture.GetTextureIndex();
			mesh.pbrMaterial.aoTextureIndex = meshAoTexture.GetTextureIndex();
			mesh.pbrMaterial.emissiveTextureIndex = meshEmissiveTexture.GetTextureIndex();

			mesh.positionBuffer.Init(device, commandList, srvCbDescriptor, modelPositions, D3D12_RESOURCE_FLAG_NONE, meshName + L" Position Buffer");
			mesh.textureCoordsBuffer.Init(device, commandList, srvCbDescriptor, modelTextureCoords, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Mesh " + meshNumber + L" Texture Coords Buffer");
			mesh.normalBuffer.Init(device, commandList, srvCbDescriptor, modelNormals, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Mesh " + meshNumber + L" Normal Buffer");
			
			if (tangentAccesor.bufferView)
			{
				mesh.tangentBuffer.Init(device, commandList, srvCbDescriptor, modelTangents, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Mesh " + meshNumber + L" Tanget Buffer");
			}

			mesh.indexBuffer.Init(device, commandList, indices, m_ModelName + L" Mesh " + meshNumber + L"Index Buffer");
			mesh.indicesCount = static_cast<uint32_t>(indices.size());

			m_Meshes.push_back(mesh);
		}

		for (const int& childrenNodeIndex : node.children)
		{
			LoadNode(device, commandList, modelDirectoryPathStr, srvCbDescriptor, childrenNodeIndex, model);
		}
	}

	void Model::UpdateTransformUI()
	{
		if (ImGui::TreeNode(WstringToString(mModelName).c_str()))
		{
			// Scale uniformally along all axises.
			ImGui::SliderFloat("Scale", &mTransform.data.scale.x, 0.1f, 10.0f);
			mTransform.data.scale = math::XMFLOAT3(mTransform.data.scale.x, mTransform.data.scale.x, mTransform.data.scale.x);

			ImGui::SliderFloat3("Translate", &mTransform.data.translate.x, -10.0f, 10.0f);
			ImGui::SliderFloat3("Rotate", &mTransform.data.rotation.x, -90.0f, 90.0f);

			ImGui::TreePop();
		}
	}

	template<typename T>
	void Model::Draw(ID3D12GraphicsCommandList* const commandList, T& renderResources)
	{
		throw std::exception("Invalid Templatized Draw function. Use one of the template specializations.");
	}

	template <>
	void Model::Draw<PBRRenderResources>(ID3D12GraphicsCommandList* const commandList, PBRRenderResources& renderResources)
	{
		for (const Mesh& mesh : m_Meshes)
		{
			auto indexBufferView = mesh.indexBuffer.GetBufferView();

			commandList->IASetIndexBuffer(&indexBufferView);

			renderResources.positionBufferIndex = mesh.positionBuffer.GetSRVIndex();
			renderResources.normalBufferIndex = mesh.normalBuffer.GetSRVIndex();
			renderResources.textureBufferIndex = mesh.textureCoordsBuffer.GetSRVIndex();
			renderResources.tangetBufferIndex = mesh.tangentBuffer.GetSRVIndex();
			
			renderResources.mvpCBufferIndex = m_TransformCBufferIndexInDescriptorHeap;

			renderResources.albedoTextureIndex = mesh.pbrMaterial.albedoTextureIndex;
			renderResources.normalTextureIndex = mesh.pbrMaterial.normalTextureIndex;
			renderResources.metalRoughnessTextureIndex = mesh.pbrMaterial.metalRoughnessTextureIndex;
			renderResources.aoTextureIndex = mesh.pbrMaterial.aoTextureIndex;
			renderResources.emissiveTextureIndex = mesh.pbrMaterial.emissiveTextureIndex;

			commandList->SetGraphicsRoot32BitConstants(0u, 64, &renderResources, 0u);

			commandList->DrawIndexedInstanced(mesh.indicesCount, 1u, 0u, 0u, 0u);
		}
	}

	template <>
	void Model::Draw<GPassRenderResources>(ID3D12GraphicsCommandList* const commandList, GPassRenderResources& renderResources)
	{
		for (const Mesh& mesh : m_Meshes)
		{
			auto indexBufferView = mesh.indexBuffer.GetBufferView();

			commandList->IASetIndexBuffer(&indexBufferView);

			renderResources.positionBufferIndex = mesh.positionBuffer.GetSRVIndex();
			renderResources.normalBufferIndex = mesh.normalBuffer.GetSRVIndex();
			renderResources.textureBufferIndex = mesh.textureCoordsBuffer.GetSRVIndex();
			renderResources.tangetBufferIndex = mesh.tangentBuffer.GetSRVIndex();

			renderResources.mvpCBufferIndex = m_TransformCBufferIndexInDescriptorHeap;

			renderResources.albedoTextureIndex = mesh.pbrMaterial.albedoTextureIndex;
			renderResources.normalTextureIndex = mesh.pbrMaterial.normalTextureIndex;
			renderResources.metalRoughnessTextureIndex = mesh.pbrMaterial.metalRoughnessTextureIndex;
			renderResources.aoTextureIndex = mesh.pbrMaterial.aoTextureIndex;
			renderResources.emissiveTextureIndex = mesh.pbrMaterial.emissiveTextureIndex;

			commandList->SetGraphicsRoot32BitConstants(0u, 64, &renderResources, 0u);

			commandList->DrawIndexedInstanced(mesh.indicesCount, 1u, 0u, 0u, 0u);
		}

	}

	template<>
	void Model::Draw<LightRenderResources>(ID3D12GraphicsCommandList* const commandList, LightRenderResources& renderResources)
	{
		for (const Mesh& mesh : m_Meshes)
		{
			auto indexBufferView = mesh.indexBuffer.GetBufferView();

			commandList->IASetIndexBuffer(&indexBufferView);

			renderResources.positionBufferIndex = mesh.positionBuffer.GetSRVIndex();
			renderResources.mvpCBufferIndex = m_TransformCBufferIndexInDescriptorHeap;

			commandList->SetGraphicsRoot32BitConstants(0u, 64, &renderResources, 0u);

			commandList->DrawIndexedInstanced(mesh.indicesCount, 1u, 0u, 0u, 0u);
		}
	}

	template<>
	void Model::Draw<ShadowPassRenderResources>(ID3D12GraphicsCommandList* const commandList, ShadowPassRenderResources& renderResources)
	{
		for (const Mesh& mesh : m_Meshes)
		{
			auto indexBufferView = mesh.indexBuffer.GetBufferView();

			commandList->IASetIndexBuffer(&indexBufferView);

			renderResources.positionBufferIndex = mesh.positionBuffer.GetSRVIndex();
			renderResources.mvpCBufferIndex = m_TransformCBufferIndexInDescriptorHeap;

			commandList->SetGraphicsRoot32BitConstants(0u, 64, &renderResources, 0u);

			commandList->DrawIndexedInstanced(mesh.indicesCount, 1u, 0u, 0u, 0u);
		}

	}
	template<>
	void Model::Draw<SkyBoxRenderResources>(ID3D12GraphicsCommandList* const commandList, SkyBoxRenderResources& renderResources)
	{
		for (const Mesh& mesh : m_Meshes)
		{
			auto indexBufferView = mesh.indexBuffer.GetBufferView();

			commandList->IASetIndexBuffer(&indexBufferView);

			renderResources.positionBufferIndex = mesh.positionBuffer.GetSRVIndex();
			renderResources.mvpCBufferIndex = m_TransformCBufferIndexInDescriptorHeap;

			commandList->SetGraphicsRoot32BitConstants(0u, 64, &renderResources, 0u);

			commandList->DrawIndexedInstanced(mesh.indicesCount, 1u, 0u, 0u, 0u);
		}
	}
}
*/