#include "Scene/Model.hpp"

#include "stb_image.h"

#include "Core/ResourceManager.hpp"
#include "Graphics/GraphicsDevice.hpp"

namespace helios::scene
{
    void TransformComponent::update()
    {
        const math::XMVECTOR scalingVector = math::XMLoadFloat3(&scale);
        const math::XMVECTOR rotationVector = math::XMLoadFloat3(&rotation);
        const math::XMVECTOR translationVector = math::XMLoadFloat3(&translate);

        const math::XMMATRIX modelMatrix = math::XMMatrixScalingFromVector(scalingVector) *
                                           math::XMMatrixRotationRollPitchYawFromVector(rotationVector) *
                                           math::XMMatrixTranslationFromVector(translationVector);

        const interlop::TransformBuffer transformBufferData = {
            .modelMatrix = modelMatrix,
            .inverseModelMatrix = DirectX::XMMatrixInverse(nullptr, modelMatrix),
        };

        transformBuffer.update(&transformBufferData);
    }

    Model::Model(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc)
        : m_modelName(modelCreationDesc.modelName)
    {
        m_modelPath = core::ResourceManager::getFullPath(modelCreationDesc.modelPath);

        // Create the transform buffer.
        m_transformComponent.transformBuffer =
            graphicsDevice->createBuffer<interlop::TransformBuffer>(gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::ConstantBuffer,
                .name = m_modelName + L" Transform Buffer",
            });

        m_transformComponent.scale = modelCreationDesc.scale;
        m_transformComponent.rotation = modelCreationDesc.rotation;
        m_transformComponent.translate = modelCreationDesc.translation;

        m_transformComponent.update();

        const std::string modelPathStr = wStringToString(m_modelPath);
        std::string modelDirectoryPathStr{};

        if (modelPathStr.find_last_of("/\\") != std::string::npos)
        {
            modelDirectoryPathStr = modelPathStr.substr(0, modelPathStr.find_last_of("/\\")) + "/";
        }

        m_modelDirectory = stringToWString(modelDirectoryPathStr);

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
                    fatalError(error);
                }

                if (!warning.empty())
                {
                    fatalError(warning);
                }
            }
        }
        else
        {
            if (!context.LoadASCIIFromFile(&model, &error, &warning, modelPathStr))
            {
                if (!error.empty())
                {
                    fatalError(error);
                }

                if (!warning.empty())
                {
                    fatalError(warning);
                }
            }
        }

        // Load samplers and materials.
        const std::jthread loadSamplerThread([&]() { loadSamplers(graphicsDevice, model); });

        const std::jthread loadMaterialThread([&]() { loadMaterials(graphicsDevice, model); });

        // Build meshes.
        const tinygltf::Scene& scene = model.scenes[model.defaultScene];

        const std::jthread loadMeshThread([&]() {
            for (const int& nodeIndex : scene.nodes)
            {
                loadNode(graphicsDevice, modelCreationDesc, nodeIndex, model);
            }
        });
    }

    void Model::render(const gfx::GraphicsContext* const graphicsContext,
                       interlop::ModelViewerRenderResources& renderResources) const
    {
        for (const Mesh& mesh : m_meshes)
        {
            graphicsContext->setIndexBuffer(mesh.indexBuffer);

            renderResources.albedoTextureIndex = m_materials[mesh.materialIndex].albedoTexture.srvIndex;
            renderResources.albedoTextureSamplerIndex =
                m_materials[mesh.materialIndex].albedoTextureSampler.samplerIndex;

            renderResources.normalBufferIndex = mesh.normalBuffer.srvIndex;
            renderResources.positionBufferIndex = mesh.positionBuffer.srvIndex;
            renderResources.textureCoordBufferIndex = mesh.textureCoordsBuffer.srvIndex;
            renderResources.transformBufferIndex = m_transformComponent.transformBuffer.cbvIndex;

            graphicsContext->set32BitGraphicsConstants(&renderResources);
            graphicsContext->drawInstanceIndexed(mesh.indicesCount);
        }
    }
    // For slight speed up in model loading, one thread will be used to load / create materials (i.e the material
    // textures), and one thread will read the mesh and fill the various accessors (position, indices, texture
    // coord's, etc) into vector so they can be loaded into buffers.
    void Model::loadNode(const gfx::GraphicsDevice* const device, const ModelCreationDesc& modelCreationDesc,
                         const uint32_t nodeIndex, const tinygltf::Model& model)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];
        if (node.mesh < 0)
        {
            // Load children immediately.
            for (const int& childrenNodeIndex : node.children)
            {
                loadNode(device, modelCreationDesc, childrenNodeIndex, model);
            }

            return;
        }

        const tinygltf::Mesh& nodeMesh = model.meshes[node.mesh];
        for (const size_t i : std::views::iota(0u, nodeMesh.primitives.size()))
        {
            Mesh mesh{};

            const std::wstring meshNumber = std::to_wstring(nodeIndex);
            const std::wstring meshName = m_modelName + L" Mesh " + std::wstring(meshNumber.c_str());

            std::vector<math::XMFLOAT3> modelPositions{};
            std::vector<math::XMFLOAT2> modelTextureCoords{};
            std::vector<math::XMFLOAT3> modelNormals{};

            std::vector<uint32_t> indices{};

            // Reference used :
            // https://github.com/mateeeeeee/Adria-DX12/blob/fc98468095bf5688a186ca84d94990ccd2f459b0/Adria/Rendering/EntityLoader.cpp.

            // Get Accesor, buffer view and buffer for each attribute (position, textureCoord, normal).
            tinygltf::Primitive primitive = nodeMesh.primitives[i];
            const tinygltf::Accessor& indexAccesor = model.accessors[primitive.indices];

            // Position data.
            const tinygltf::Accessor& positionAccesor = model.accessors[primitive.attributes["POSITION"]];
            const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccesor.bufferView];
            const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];

            const int positionByteStride = positionAccesor.ByteStride(positionBufferView);
            const uint8_t* positions = &positionBuffer.data[positionBufferView.byteOffset + positionAccesor.byteOffset];

            // TextureCoord data.
            const tinygltf::Accessor& textureCoordAccesor = model.accessors[primitive.attributes["TEXCOORD_0"]];
            const tinygltf::BufferView& textureCoordBufferView = model.bufferViews[textureCoordAccesor.bufferView];
            const tinygltf::Buffer& textureCoordBuffer = model.buffers[textureCoordBufferView.buffer];
            const int textureCoordBufferStride = textureCoordAccesor.ByteStride(textureCoordBufferView);
            const uint8_t* texcoords =
                &textureCoordBuffer.data[textureCoordBufferView.byteOffset + textureCoordAccesor.byteOffset];

            // Normal data.
            const tinygltf::Accessor& normalAccesor = model.accessors[primitive.attributes["NORMAL"]];
            const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccesor.bufferView];
            const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
            const int normalByteStride = normalAccesor.ByteStride(normalBufferView);
            const uint8_t* normals = &normalBuffer.data[normalBufferView.byteOffset + normalAccesor.byteOffset];

            // Get the index buffer data.
            const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccesor.bufferView];
            const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
            const int indexByteStride = indexAccesor.ByteStride(indexBufferView);
            const uint8_t* indexes = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccesor.byteOffset;

            {
                const std::jthread vertexAttributesThread([&]() {
                    {
                        // Fill in the vertices array.
                        for (size_t i : std::views::iota(0u, positionAccesor.count))
                        {
                            const math::XMFLOAT3 position = {
                                (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[0],
                                (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[1],
                                (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[2],
                            };

                            const math::XMFLOAT2 textureCoord = {
                                (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[0],
                                (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1],
                            };

                            const math::XMFLOAT3 normal = {
                                (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0],
                                (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1],
                                (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2],
                            };

                            modelPositions.emplace_back(position);
                            modelTextureCoords.emplace_back(textureCoord);
                            modelNormals.emplace_back(normal);
                        }
                    }
                });

                const std::jthread indexBufferDataThread([&]() {
                    // Fill indices array.
                    for (const size_t i : std::views::iota(0u, indexAccesor.count))
                    {
                        if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                        {
                            indices.push_back(static_cast<uint32_t>(
                                (reinterpret_cast<uint16_t const*>(indexes + (i * indexByteStride)))[0]));
                        }
                        else if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                        {
                            indices.push_back(static_cast<uint32_t>(
                                (reinterpret_cast<uint32_t const*>(indexes + (i * indexByteStride)))[0]));
                        }
                    }
                });
            }

            mesh.positionBuffer = device->createBuffer<math::XMFLOAT3>(
                gfx::BufferCreationDesc{
                    .usage = gfx::BufferUsage::StructuredBuffer,
                    .name = meshName + L" position buffer",
                },
                modelPositions);

            mesh.textureCoordsBuffer = device->createBuffer<math::XMFLOAT2>(
                gfx::BufferCreationDesc{
                    .usage = gfx::BufferUsage::StructuredBuffer,
                    .name = meshName + L" texture coord buffer",
                },
                modelTextureCoords);

            mesh.normalBuffer = device->createBuffer<math::XMFLOAT3>(
                gfx::BufferCreationDesc{
                    .usage = gfx::BufferUsage::StructuredBuffer,
                    .name = meshName + L" normal buffer",
                },
                modelNormals);

            mesh.indexBuffer = device->createBuffer<uint32_t>(
                gfx::BufferCreationDesc{
                    .usage = gfx::BufferUsage::StructuredBuffer,
                    .name = meshName + L" index buffer",
                },
                indices);

            mesh.indicesCount = static_cast<uint32_t>(indices.size());

            mesh.materialIndex = primitive.material;

            m_meshes.push_back(mesh);
        }

        for (const int& childrenNodeIndex : node.children)
        {
            loadNode(device, modelCreationDesc, childrenNodeIndex, model);
        }
    }

    // Reference : https://github.com/syoyo/tinygltf/blob/master/examples/dxview/src/Viewer.cc
    void Model::loadSamplers(const gfx::GraphicsDevice* const graphicsDevice, const tinygltf::Model& model)
    {
        m_samplers.resize(model.samplers.size());

        size_t index{0};

        for (const tinygltf::Sampler& sampler : model.samplers)
        {
            gfx::SamplerCreationDesc samplerCreationDesc{};

            switch (sampler.minFilter)
            {
            case TINYGLTF_TEXTURE_FILTER_NEAREST: {
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                }
                else
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                }
            }
            break;

            case TINYGLTF_TEXTURE_FILTER_LINEAR: {
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                }
                else
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                }
            }
            break;

            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: {
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                }
                else
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                }
            }
            break;

            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: {
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                }
                else
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                }
            }
            break;

            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: {
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                }
                else
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                }
            }
            break;

            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: {
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                }
                else
                {
                    samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                }
            }
            break;

            default: {
                samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
            }
            break;
            }

            auto toTextureAddressMode = [](int wrap) {
                switch (wrap)
                {
                case TINYGLTF_TEXTURE_WRAP_REPEAT: {
                    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                }
                break;

                case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: {
                    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                }
                break;

                case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: {
                    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                }
                break;

                default: {
                    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                }
                break;
                }
            };

            samplerCreationDesc.samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
            samplerCreationDesc.samplerDesc.AddressU = toTextureAddressMode(sampler.wrapS);
            samplerCreationDesc.samplerDesc.AddressV = toTextureAddressMode(sampler.wrapT);
            samplerCreationDesc.samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerCreationDesc.samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerCreationDesc.samplerDesc.MinLOD = 0.0f;
            samplerCreationDesc.samplerDesc.MipLODBias = 0.0f;
            samplerCreationDesc.samplerDesc.MaxAnisotropy = 16;
            samplerCreationDesc.samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

            m_samplers[index++] = graphicsDevice->createSampler(samplerCreationDesc);
        }
    }

    void Model::loadMaterials(const gfx::GraphicsDevice* graphicsDevice, const tinygltf::Model& model)
    {
        const auto createTexture = [&](const tinygltf::Image& image,
                                       const gfx::TextureCreationDesc& paramTextureCreationDesc) {
            const std::string texturePath = wStringToString(m_modelDirectory) + image.uri;

            gfx::TextureCreationDesc textureCreationDesc = paramTextureCreationDesc;

            int32_t width{}, height{};
            const unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, nullptr, 4);
            if (!data)
            {
                fatalError(std::format("Failed to load texture from path : {}", texturePath));
            }

            // determine max mip levels possible.

            textureCreationDesc.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);

            textureCreationDesc.width = static_cast<uint32_t>(width);
            textureCreationDesc.height = static_cast<uint32_t>(height);

            return graphicsDevice->createTexture(textureCreationDesc, (std::byte*)data);
        };

        size_t index{0};
        m_materials.resize(model.materials.size());

        for (const tinygltf::Material& material : model.materials)
        {
            PBRMaterial pbrMaterial{};

            {
                const std::jthread albedoTextureThread([&]() {
                    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
                    {
                        const tinygltf::Texture& albedoTexture =
                            model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                        const tinygltf::Image& albedoImage = model.images[albedoTexture.source];

                        pbrMaterial.albedoTexture =
                            createTexture(albedoImage, gfx::TextureCreationDesc{
                                                           .usage = gfx::TextureUsage::TextureFromData,
                                                           .format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                           .mipLevels = 6u,
                                                           .name = m_modelName + L" albedo texture",
                                                       });
                        pbrMaterial.albedoTextureSampler = m_samplers[albedoTexture.sampler];
                    }
                });

                const std::jthread metalRoughnessThread([&]() {
                    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
                    {

                        const tinygltf::Texture& metalRoughnessTexture =
                            model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                        const tinygltf::Image& metalRoughnessImage = model.images[metalRoughnessTexture.source];

                        pbrMaterial.metalRoughnessTexture =
                            createTexture(metalRoughnessImage, gfx::TextureCreationDesc{
                                                                   .usage = gfx::TextureUsage::TextureFromData,
                                                                   .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                   .mipLevels = 4u,
                                                                   .name = m_modelName + L" metal roughness texture",
                                                               });
                        pbrMaterial.metalRoughnessTextureSampler = m_samplers[metalRoughnessTexture.sampler];
                    }
                });

                const std::jthread normalTextureThread([&]() {
                    if (material.normalTexture.index >= 0)
                    {

                        const tinygltf::Texture& normalTexture = model.textures[material.normalTexture.index];
                        const tinygltf::Image& normalImage = model.images[normalTexture.source];

                        pbrMaterial.normalTexture =
                            createTexture(normalImage, gfx::TextureCreationDesc{
                                                           .usage = gfx::TextureUsage::TextureFromData,
                                                           .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                           .mipLevels = 2u,
                                                           .name = m_modelName + L" normal texture",
                                                       });

                        pbrMaterial.normalTextureSampler = m_samplers[normalTexture.sampler];
                    }
                });

                const std::jthread occlusionTextureThead([&]() {
                    if (material.occlusionTexture.index >= 0)
                    {

                        const tinygltf::Texture& aoTexture = model.textures[material.occlusionTexture.index];
                        const tinygltf::Image& aoImage = model.images[aoTexture.source];

                        pbrMaterial.aoTexture = createTexture(aoImage, gfx::TextureCreationDesc{
                                                                           .usage = gfx::TextureUsage::TextureFromData,
                                                                           .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                           .mipLevels = 4u,
                                                                           .name = m_modelName + L" occlusion texture",
                                                                       });
                        pbrMaterial.aoTextureSampler = m_samplers[aoTexture.sampler];
                    }
                });

                const std::jthread emissiveTextureThead([&]() {
                    if (material.emissiveTexture.index >= 0)
                    {

                        const tinygltf::Texture& emissiveTexture = model.textures[material.emissiveTexture.index];
                        const tinygltf::Image& emissiveImage = model.images[emissiveTexture.source];

                        pbrMaterial.emissiveTexture =
                            createTexture(emissiveImage, gfx::TextureCreationDesc{
                                                             .usage = gfx::TextureUsage::TextureFromData,
                                                             .format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                             .mipLevels = 4u,
                                                             .name = m_modelName + L" emissive texture",
                                                         });
                        pbrMaterial.emissiveTextureSampler = m_samplers[emissiveTexture.sampler];
                    }
                });
            }

            m_materials[index++] = std::move(pbrMaterial);
        }
    }
} // namespace helios::scene