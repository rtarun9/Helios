#include "Common/BindlessRS.hlsli"

#include "Model.hpp"

#include "stb_image.h"

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace Microsoft::WRL;
using namespace DirectX;

namespace helios::scene
{
	Model::Model(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc)
	{
		if (modelCreationDesc.modelPath.find(utility::ResourceManager::GetAssetPath(L"")) == std::wstring::npos)
		{
			mModelPath = utility::ResourceManager::GetAssetPath(modelCreationDesc.modelPath);
		}
		else
		{
			mModelPath = modelCreationDesc.modelPath;
		};

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

		if (modelPathStr.find(".glb") != std::string::npos)
		{
			if (!context.LoadBinaryFromFile(&model, &error, &warning, modelPathStr))
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

		}

		// Load samplers.
		std::thread loadSamplerThread([&]()
		{
			LoadSamplers(device, model);
		});
		
		// Load textures and materials.
		std::thread loadMaterialThread([&]()
		{
			LoadMaterials(device, model);
		});
		
		// Build meshes.
		tinygltf::Scene& scene = model.scenes[model.defaultScene];
		
		std::thread loadMeshThread([&]()
		{
			for (const int& nodeIndex : scene.nodes)
			{
				LoadNode(device, modelCreationDesc, nodeIndex, model);
			}
		});
		
		loadSamplerThread.join();
		loadMaterialThread.join();
		loadMeshThread.join();
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
			std::vector<math::XMFLOAT3> modelBiTangents{};

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
					(reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1],
				};

				math::XMFLOAT3 normal
				{
					(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0],
					(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1],
					(reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2],
				};

				math::XMFLOAT4 tangent{};
				math::XMFLOAT3 biTangent{};

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
				else
				{
					// note(rtarun9) : This code works, but looks very pixelated in shaders. Utils.hlsli calculates tangents instead.
					math::XMVECTOR normalVector = math::XMLoadFloat3(&normal);
					math::XMVECTOR tangentVector = math::XMVector3Cross(normalVector, math::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
					tangentVector = math::XMVectorLerp(math::XMVector3Cross(normalVector, math::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)), tangentVector, math::XMVectorGetX(math::XMVector3Dot(tangentVector, tangentVector)));
					tangentVector = math::XMVector3Normalize(tangentVector);
					math::XMFLOAT3 tangent3Vector{};
					math::XMStoreFloat3(&tangent3Vector, tangentVector);
					tangent = { tangent3Vector.x, tangent3Vector.y, tangent3Vector.z, 1.0f };
				}

				// Calculate tangent.
				// tangent.z is just a constant (-1 or 1) to indicate the handedness.
				math::XMFLOAT3 tangent3Vector = { tangent.x, tangent.y, tangent.z };
				math::XMVECTOR biTangentVector = math::XMVectorScale(math::XMVector3Cross(XMLoadFloat3(&normal), XMLoadFloat3(&tangent3Vector)), tangent.z);
				math::XMStoreFloat3(&biTangent, biTangentVector);

				modelPositions.emplace_back(position);
				modelTextureCoords.emplace_back(textureCoord);
				modelNormals.emplace_back(normal);
				modelTangents.emplace_back(tangent);
				modelBiTangents.emplace_back(biTangent);
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

			
			gfx::BufferCreationDesc tangentBufferCreationDesc
			{
				.usage = gfx::BufferUsage::StructuredBuffer,
				.name = meshName + L" tangent buffer",
			};

			mesh.tangentBuffer = std::make_shared<gfx::Buffer>(device->CreateBuffer<DirectX::XMFLOAT4>(tangentBufferCreationDesc, modelTangents));
			

			gfx::BufferCreationDesc indexBufferCreationDesc
			{
				.usage = gfx::BufferUsage::IndexBuffer,
				.name = meshName + L" index buffer",
			};

			mesh.indexBuffer = std::make_shared<gfx::Buffer>(device->CreateBuffer<uint32_t>(indexBufferCreationDesc, indices));

			mesh.indicesCount = static_cast<uint32_t>(indices.size());

			mesh.materialIndex = primitive.material;

			mMeshes.push_back(mesh);
		}

		for (const int& childrenNodeIndex : node.children)
		{
			LoadNode(device, modelCreationDesc, childrenNodeIndex, model);
		}
	}

	// Reference : https://github.com/syoyo/tinygltf/blob/master/examples/dxview/src/Viewer.cc
	void Model::LoadSamplers(const gfx::Device* device, tinygltf::Model& model)
	{
		mSamplers.resize(model.samplers.size());

		size_t index{ 0 };

		for (tinygltf::Sampler& sampler : model.samplers) 
		{
			gfx::SamplerCreationDesc samplerCreationDesc{};

			switch (sampler.minFilter) 
			{
				case TINYGLTF_TEXTURE_FILTER_NEAREST:
				{
					if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
					}
					else
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
					}
				}break;

				case TINYGLTF_TEXTURE_FILTER_LINEAR:
				{
					if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
					}
					else
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
					}
				}break;

				case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
				{
					if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
					}
					else
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
					}
				}break;

		
				case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
				{
					if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
					}
					else
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
					}
				}break;

				case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
				{
					if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
					}
					else
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
					}
				}break;

				case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
				{
					if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
					}
					else
					{
						samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
					}
				}break;
				
				default:
				{
					samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
				}
				break;
			}

			auto toTextureAddressMode = [](int wrap) 
			{
				switch (wrap) 
				{
					case TINYGLTF_TEXTURE_WRAP_REPEAT:
					{
						return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
					}break;

					case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
					{
						return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					}break;

					case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
					{
						return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
					}break;
					
					default:
					{
						return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
					}break;
				}
			};

			samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
			samplerCreationDesc.samplerDesc.AddressU = toTextureAddressMode(sampler.wrapS);
			samplerCreationDesc.samplerDesc.AddressV = toTextureAddressMode(sampler.wrapT);
			samplerCreationDesc.samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerCreationDesc.samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			samplerCreationDesc.samplerDesc.MinLOD = 0.0f;
			samplerCreationDesc.samplerDesc.MipLODBias = 0.0f;
			samplerCreationDesc.samplerDesc.MaxAnisotropy =16;
			samplerCreationDesc.samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			
			uint32_t samplerIndex = device->CreateSampler(samplerCreationDesc);
			mSamplers[index++] = samplerIndex;
		}
	}

	void Model::LoadMaterials(const gfx::Device* device, tinygltf::Model& model)
	{
		auto CreateTexture = [&](tinygltf::Image& image, gfx::TextureCreationDesc& textureCreationDesc)
		{
			std::string texturePath = WstringToString(mModelDirectory) + image.uri;

			int width{}, height{};
			unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, nullptr, 4);
			if (!data)
			{
				ErrorMessage(L"Failed to load texture from path : " + StringToWString(texturePath));
			}

			// Create max mip levels possible.

			textureCreationDesc.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
	
			textureCreationDesc.dimensions = { (uint32_t)width, (uint32_t)height };

			return std::move(std::make_unique<gfx::Texture>(device->CreateTexture(textureCreationDesc, data)));
		};

		size_t index{ 0 };
		mMaterials.resize(model.materials.size());

		for (const tinygltf::Material& material : model.materials)
		{
			if (mModelName == L"Sponza")
			{
				auto x = 3;
			}
			PBRMaterial pbrMaterial{};

			std::thread albedoTextureThread([&]() 
			{
				if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
				{
					gfx::TextureCreationDesc albedoTextureCreationDesc
					{
						.usage = gfx::TextureUsage::TextureFromData,
						.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
						.mipLevels = 6u,
						.name = mModelName + L" albedo texture"
					};

					tinygltf::Texture& albedoTexture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
					tinygltf::Image& albedoImage = model.images[albedoTexture.source];

					pbrMaterial.albedoTexture = CreateTexture(albedoImage, albedoTextureCreationDesc);
					pbrMaterial.albedoTextureSamplerIndex = mSamplers[albedoTexture.sampler];
				}

			});
			
			std::thread metalRoughnessTextureThread([&]()
			{
				if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
				{
					gfx::TextureCreationDesc metalRoughnessTextureCreationDesc
					{
						.usage = gfx::TextureUsage::TextureFromData,
						.format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.mipLevels = 4u,
						.name = mModelName + L" metal roughness texture"
					};

					tinygltf::Texture& metalRoughnessTexture = model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
					tinygltf::Image& metalRoughnessImage = model.images[metalRoughnessTexture.source];

					pbrMaterial.metalRoughnessTexture = CreateTexture(metalRoughnessImage, metalRoughnessTextureCreationDesc);
					pbrMaterial.metalRoughnessTextureSamplerIndex = mSamplers[metalRoughnessTexture.sampler];
				}
			});

			std::thread normalTextureThread([&]()
			{
				if (material.normalTexture.index >= 0)
				{
					gfx::TextureCreationDesc normalTextureCreationDesc
					{
						.usage = gfx::TextureUsage::TextureFromData,
						.format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.mipLevels = 2u,
						.name = mModelName + L" normal texture",
					};

					tinygltf::Texture& normalTexture = model.textures[material.normalTexture.index];
					tinygltf::Image& normalImage = model.images[normalTexture.source];

					pbrMaterial.normalTexture = CreateTexture(normalImage, normalTextureCreationDesc);
					pbrMaterial.normalTextureSamplerIndex = mSamplers[normalTexture.sampler];
				}
			});

			std::thread occlusionTextureThread([&]()
			{
				if (material.occlusionTexture.index >= 0)
				{
					gfx::TextureCreationDesc occlusionTextureCreationDesc
					{
						.usage = gfx::TextureUsage::TextureFromData,
						.format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.mipLevels = 4u,
						.name = mModelName + L" occlusion texture",
					};

					tinygltf::Texture& aoTexture = model.textures[material.occlusionTexture.index];
					tinygltf::Image& aoImage = model.images[aoTexture.source];

					pbrMaterial.aoTexture = CreateTexture(aoImage, occlusionTextureCreationDesc);
					pbrMaterial.aoTextureSamplerIndex = mSamplers[aoTexture.sampler];
				}
			});
			
			std::thread emissiveTextureThread([&]()
			{
				if (material.emissiveTexture.index >= 0)
				{
					gfx::TextureCreationDesc emissiveTextureCreationDesc
					{
						.usage = gfx::TextureUsage::TextureFromData,
						.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
						.mipLevels = 4u,
						.name = mModelName + L" emissive texture",
					};

					tinygltf::Texture& emissiveTexture = model.textures[material.emissiveTexture.index];
					tinygltf::Image& emissiveImage = model.images[emissiveTexture.source];

					pbrMaterial.emissiveTexture = CreateTexture(emissiveImage, emissiveTextureCreationDesc);
					pbrMaterial.emissiveTextureSamplerIndex = mSamplers[emissiveTexture.sampler];
				}
			});

			albedoTextureThread.join();
			metalRoughnessTextureThread.join();
			normalTextureThread.join();
			occlusionTextureThread.join();
			emissiveTextureThread.join();

			mMaterials[index++] = std::move(pbrMaterial);
		}
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
				.tangentBufferIndex = gfx::Buffer::GetSrvIndex(mesh.tangentBuffer.get()),
				.biTangentBufferIndex = gfx::Buffer::GetSrvIndex(mesh.biTangentBuffer.get()),

				.transformBufferIndex = gfx::Buffer::GetCbvIndex(mTransform.transformBuffer.get()),
				.sceneBufferIndex = sceneRenderResources.sceneBufferIndex,
				.lightBufferIndex = sceneRenderResources.lightBufferIndex,

				.albedoTextureIndex = gfx::Texture::GetSrvIndex(mMaterials[mesh.materialIndex].albedoTexture.get()),
				.albedoTextureSamplerIndex = mMaterials[mesh.materialIndex].albedoTextureSamplerIndex,

				.metalRoughnessTextureIndex = gfx::Texture::GetSrvIndex(mMaterials[mesh.materialIndex].metalRoughnessTexture.get()),
				.metalRoughnessTextureSamplerIndex = mMaterials[mesh.materialIndex].metalRoughnessTextureSamplerIndex,
				
				.normalTextureIndex = gfx::Texture::GetSrvIndex(mMaterials[mesh.materialIndex].normalTexture.get()),
				.normalTextureSamplerIndex = mMaterials[mesh.materialIndex].normalTextureSamplerIndex,

				.aoTextureIndex = gfx::Texture::GetSrvIndex(mMaterials[mesh.materialIndex].aoTexture.get()),
				.aoTextureSamplerIndex = mMaterials[mesh.materialIndex].aoTextureSamplerIndex,

				.emissiveTextureIndex = gfx::Texture::GetSrvIndex(mMaterials[mesh.materialIndex].emissiveTexture.get()),
				.emissiveTextureSamplerIndex = mMaterials[mesh.materialIndex].emissiveTextureSamplerIndex
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

	void Model::Render(const gfx::GraphicsContext* graphicsContext, SkyBoxRenderResources& skyBoxrenderResources)
	{
		for (const Mesh& mesh : mMeshes)
		{
			graphicsContext->SetIndexBuffer(mesh.indexBuffer.get());

			skyBoxrenderResources.positionBufferIndex = gfx::Buffer::GetSrvIndex(mesh.positionBuffer.get());

			graphicsContext->Set32BitGraphicsConstants(&skyBoxrenderResources);

			graphicsContext->DrawInstanceIndexed(mesh.indicesCount);
		}
	}
}
