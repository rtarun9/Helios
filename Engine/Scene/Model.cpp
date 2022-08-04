#include "Pch.hpp"

#include "Core/Helpers.hpp"

#include "Common/BindlessRS.hlsli"

#include "Model.hpp"

#include "STB/stb_image.h"

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace Microsoft::WRL;
using namespace DirectX;

namespace helios::scene
{
	Model::Model(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc)
	{
		mModelPath = modelCreationDesc.modelPath;
		mModelName = modelCreationDesc.modelName;

		gfx::BufferCreationDesc transformBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = mModelName + L" Transform Buffer",
		};

		mTransform.transformBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<TransformBuffer>(transformBufferCreationDesc, std::span<TransformBuffer, 0u>{}));

		std::string modelPathStr = WstringToString(mModelPath);
		std::string modelDirectoryPathStr{};

		if (modelPathStr.find_last_of("/\\") != std::string::npos)
		{
			modelDirectoryPathStr =  modelPathStr.substr(0, modelPathStr.find_last_of("/\\")) + "/";
		}
		
		mModelDirectory = StringToWString(modelDirectoryPathStr);

		std::string warning{};
		std::string error{};

		tinygltf::TinyGLTF context{};

		tinygltf::Model model{};

		std::vector<uint8_t> fileContents{};
		ReadFile(mModelPath, fileContents);

		if (modelPathStr.find(".glb") != std::string::npos)
		{
			if (!context.LoadBinaryFromMemory(&model, &error, &warning, fileContents.data(), static_cast<unsigned int>(fileContents.size()), modelDirectoryPathStr))
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
		}
		else
		{
			if (!context.LoadASCIIFromString(&model, &error, &warning, reinterpret_cast<const char*>(&fileContents.at(0)), static_cast<unsigned int>(fileContents.size()), modelDirectoryPathStr))
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

		}

		// Build meshes.
		tinygltf::Scene& scene = model.scenes[model.defaultScene];

		for (const int& nodeIndex : scene.nodes)
		{
			LoadNode(device, modelCreationDesc, nodeIndex, model);
		}
	}

	// For slight speed up in model loading, one thread will be used to load / create materials (i.e the material textures),
	// and one thread will read the mesh and fill the various accessors (position, indices, texture coord's, etc) into vector so they can be loaded into buffers.
	void Model::LoadNode(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc, uint32_t nodeIndex, tinygltf::Model& model)
	{
		tinygltf::Node& node = model.nodes[nodeIndex];
		if (node.mesh < 0)
		{
			// Load children immediatly, as it may have some.
			for (const int& childrenNodeIndex : node.children)
			{
				LoadNode(device, modelCreationDesc, childrenNodeIndex, model);
			}

			return;
		}

		tinygltf::Mesh& nodeMesh = model.meshes[node.mesh];
		for (size_t i = 0; i < nodeMesh.primitives.size(); ++i)
		{
			Mesh mesh{};

			const std::wstring meshNumber = std::to_wstring(nodeIndex);
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

			std::mutex textureCreationMutexLock{};

			std::thread materialLoadingThread([&]()
			{
				// Load material textures. 
				// note(rtarun9) : Not complete implementation, but basic framework).
				tinygltf::Material gltfPBRMaterial = model.materials[primitive.material];

				std::thread albedoTextureThread([&]()
				{
					if (gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
					{
						tinygltf::Texture& albedoTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index];
						tinygltf::Image& albedoImage = model.images[albedoTexture.source];

						std::wstring albedoTexturePath = mModelDirectory + StringToWString(albedoImage.uri);
						if (!albedoImage.uri.empty())
						{

							gfx::TextureCreationDesc albedoTextureCreationDesc
							{
								.usage = gfx::TextureUsage::TextureFromData,
								.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
								.mipLevels = 6u,
								.name = meshName + L" albedo texture",
								.path = albedoTexturePath,
							};

							unsigned char* data{};
							uint32_t componentCount{ 4u };

							LoadTexture(data, componentCount, albedoTextureCreationDesc);
							std::lock_guard<std::mutex> mutexGuard(textureCreationMutexLock);
							mesh.pbrMaterial.albedoTexture = std::make_shared<gfx::Texture>(device->CreateTexture(albedoTextureCreationDesc, data));
						}
					}
				});
				
				std::thread normalTextureThread([&]()
				{
					if (gltfPBRMaterial.normalTexture.index >= 0)
					{
						tinygltf::Texture& normalTexture = model.textures[gltfPBRMaterial.normalTexture.index];
						tinygltf::Image& normalImage = model.images[normalTexture.source];

						std::wstring normalTexturePath = mModelDirectory + StringToWString(normalImage.uri);

						if (!normalImage.uri.empty())
						{
							gfx::TextureCreationDesc normalTextureCreationDesc
							{
								.usage = gfx::TextureUsage::TextureFromData,
								.format = DXGI_FORMAT_R8G8B8A8_UNORM,
								.mipLevels = 4u,
								.name = meshName + L" normal texture",
								.path = normalTexturePath,
							};

							unsigned char* data{};
							uint32_t componentCount{ 4u };

							LoadTexture(data, componentCount, normalTextureCreationDesc);
							std::lock_guard<std::mutex> mutexGuard(textureCreationMutexLock);
							mesh.pbrMaterial.normalTexture = std::make_shared<gfx::Texture>(device->CreateTexture(normalTextureCreationDesc, data));
						}
					}
				});
				
				std::thread metallicRoughnessTextureThread([&]()
				{
					if (gltfPBRMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
					{
						tinygltf::Texture& metalRoughnessTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
						tinygltf::Image& metalRoughnessImage = model.images[metalRoughnessTexture.source];

						std::wstring metallicRoughnessTexturePath = mModelDirectory + StringToWString(metalRoughnessImage.uri);
						if (!metalRoughnessImage.uri.empty())
						{
							gfx::TextureCreationDesc metalRoughnessTextureCreationDesc
							{
								.usage = gfx::TextureUsage::TextureFromData,
								.format = DXGI_FORMAT_R8G8B8A8_UNORM,
								.mipLevels = 4u,
								.name = meshName + L" metal roughness texture",
								.path = metallicRoughnessTexturePath,
							};
							unsigned char* data{};
							uint32_t componentCount{ 4u };

							LoadTexture(data, componentCount, metalRoughnessTextureCreationDesc);
							std::lock_guard<std::mutex> mutexGuard(textureCreationMutexLock);
							mesh.pbrMaterial.metalRoughnessTexture = std::make_shared<gfx::Texture>(device->CreateTexture(metalRoughnessTextureCreationDesc, data));
						}
					}
				});

				std::thread occlusionTextureThread([&]()
				{
					if (gltfPBRMaterial.occlusionTexture.index >= 0)
					{
						tinygltf::Texture& aoTexture = model.textures[gltfPBRMaterial.occlusionTexture.index];
						tinygltf::Image& aoImage = model.images[aoTexture.source];

						std::wstring occlusionTexturePath = mModelDirectory + StringToWString(aoImage.uri);
						if (!aoImage.uri.empty())
						{
							gfx::TextureCreationDesc occlusionTextureCreationDesc
							{
								.usage = gfx::TextureUsage::TextureFromData,
								.format = DXGI_FORMAT_R8G8B8A8_UNORM,
								.mipLevels = 4u,
								.name = meshName + L" occlusion texture",
								.path = occlusionTexturePath,
							};

							unsigned char* data{};
							uint32_t componentCount{ 4u };

							LoadTexture(data, componentCount, occlusionTextureCreationDesc);
							std::lock_guard<std::mutex> mutexGuard(textureCreationMutexLock);
							mesh.pbrMaterial.aoTexture = std::make_shared<gfx::Texture>(device->CreateTexture(occlusionTextureCreationDesc, data));
						}
					}
				});
				
				std::thread emmisiveTextureThread([&]()
				{
					if (gltfPBRMaterial.emissiveTexture.index >= 0)
					{
						tinygltf::Texture& emissiveTexture = model.textures[gltfPBRMaterial.emissiveTexture.index];
						tinygltf::Image& emissiveImage = model.images[emissiveTexture.source];

						std::wstring emissiveTexturePath = mModelDirectory + StringToWString(emissiveImage.uri);
						if (!emissiveImage.uri.empty())
						{
							gfx::TextureCreationDesc emissiveTextureCreationDesc
							{
								.usage = gfx::TextureUsage::TextureFromData,
								.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
								.mipLevels = 4u,
								.name = meshName + L" emissive texture",
								.path = emissiveTexturePath ,
							};

							unsigned char* data{};
							uint32_t componentCount{ 4u };

							LoadTexture(data, componentCount, emissiveTextureCreationDesc);
							std::lock_guard<std::mutex> mutexGuard(textureCreationMutexLock);
							mesh.pbrMaterial.emissiveTexture = std::make_shared<gfx::Texture>(device->CreateTexture(emissiveTextureCreationDesc, data));
						}
					}	
				});

				albedoTextureThread.join();
				normalTextureThread.join();
				metallicRoughnessTextureThread.join();
				occlusionTextureThread.join();
				emmisiveTextureThread.join();
			});
				
			// DOnt create tangent buffer if no tangent data exist.
			bool tangentDataExist = false;

			std::thread bufferDataLoadingThread([&]()
			{
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

				tangentDataExist = tangentAccesor.bufferView;

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
						(reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1],
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
			});


			// Create all the buffers (wait for buffer data loading thread and material loading thread before proceeding).
			bufferDataLoadingThread.join();
			materialLoadingThread.join();

			gfx::BufferCreationDesc positionBufferCreationDesc
			{
				.usage = gfx::BufferUsage::StructuredBuffer,
				.name = meshName + L" position buffer",
			};

			mesh.positionBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT3>(positionBufferCreationDesc, modelPositions));

			gfx::BufferCreationDesc textureCoordsBufferCreationDesc
			{
				.usage = gfx::BufferUsage::StructuredBuffer,
				.name = meshName + L" texture coords buffer",
			};

			mesh.textureCoordsBuffer  = std::make_shared<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT2>(textureCoordsBufferCreationDesc, modelTextureCoords));

			gfx::BufferCreationDesc normalBufferCreationDesc
			{
				.usage = gfx::BufferUsage::StructuredBuffer,
				.name = meshName + L" normal buffer",
			};

			mesh.normalBuffer = std::make_shared<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT3>(normalBufferCreationDesc, modelNormals));

			if (tangentDataExist)
			{
				gfx::BufferCreationDesc tangentBufferCreationDesc
				{
					.usage = gfx::BufferUsage::StructuredBuffer,
					.name = meshName + L" tangent buffer",
				};

				mesh.tangentBuffer = std::make_shared<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT4>(tangentBufferCreationDesc, modelTangents));
			}

			gfx::BufferCreationDesc indexBufferCreationDesc
			{
				.usage = gfx::BufferUsage::IndexBuffer,
				.name = meshName + L" index buffer",
			};

			mesh.indexBuffer = std::make_shared<gfx::Buffer>(device->CreateBuffer<uint32_t>(indexBufferCreationDesc, indices));

			mesh.indicesCount = static_cast<uint32_t>(indices.size());

			mMeshes.push_back(mesh);
		}

		for (const int& childrenNodeIndex : node.children)
		{
			LoadNode(device, modelCreationDesc, childrenNodeIndex, model);
		}
	}
	
	// Function for loading texture from path (so that texture's can be loaded in parallel).
	// The textureCreationDesc will be filled with appropirate dimensions in this function as well.
	void Model::LoadTexture(unsigned char*& data, uint32_t& componentCount, gfx::TextureCreationDesc& textureCreationDesc)
	{
		int width{}, height{};

		data = stbi_load(WstringToString(textureCreationDesc.path).c_str(), &width, &height, nullptr, componentCount);
		if (!data)
		{
			ErrorMessage(L"Failed to load texture from path : " + textureCreationDesc.path);
		}

		textureCreationDesc.dimensions.x = width;
		textureCreationDesc.dimensions.y = height;
	}

	void Model::Render(const gfx::GraphicsContext* graphicsContext, const SceneRenderResources& sceneRenderResources)
	{
		for (const Mesh& mesh : mMeshes)
		{
			graphicsContext->SetIndexBuffer(mesh.indexBuffer.get());

			PBRRenderResources pbrRenderResources
			{
				.positionBufferIndex = gfx::Buffer::GetSrvIndex(mesh.positionBuffer.get()),
				.textureBufferIndex = gfx::Buffer::GetSrvIndex(mesh.textureCoordsBuffer.get()),
				.normalBufferIndex = gfx::Buffer::GetSrvIndex(mesh.normalBuffer.get()),
				.tangetBufferIndex = gfx::Buffer::GetSrvIndex(mesh.tangentBuffer.get()),

				.transformBufferIndex = gfx::Buffer::GetCbvIndex(mTransform.transformBuffer.get()),
				.sceneBufferIndex = sceneRenderResources.sceneBufferIndex,
				.lightBufferIndex = sceneRenderResources.lightBufferIndex,

				.albedoTextureIndex = gfx::Texture::GetSrvIndex(mesh.pbrMaterial.albedoTexture.get()),
				.metalRoughnessTextureIndex = gfx::Texture::GetSrvIndex(mesh.pbrMaterial.metalRoughnessTexture.get()),
				.normalTextureIndex = gfx::Texture::GetSrvIndex(mesh.pbrMaterial.normalTexture.get()),
				.aoTextureIndex = gfx::Texture::GetSrvIndex(mesh.pbrMaterial.aoTexture.get()),
				.emissiveTextureIndex = gfx::Texture::GetSrvIndex(mesh.pbrMaterial.emissiveTexture.get()),
			};


			graphicsContext->Set32BitGraphicsConstants(&pbrRenderResources);

			graphicsContext->DrawInstanceIndexed(mesh.indicesCount);
		}
	}

	// See Light.hpp for why this function takes a ref and not const ref to the render resources struct.
	void Model::Render(const gfx::GraphicsContext* graphicsContext, LightRenderResources& lightRenderResources)
	{
		for (const Mesh& mesh : mMeshes)
		{
			graphicsContext->SetIndexBuffer(mesh.indexBuffer.get());

			lightRenderResources.positionBufferIndex = gfx::Buffer::GetSrvIndex(mesh.positionBuffer.get());

			graphicsContext->Set32BitGraphicsConstants(&lightRenderResources);

			graphicsContext->DrawInstanceIndexed(mesh.indicesCount, TOTAL_POINT_LIGHTS);
		}
	}
}
