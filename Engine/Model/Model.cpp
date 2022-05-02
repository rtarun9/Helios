#include "Pch.hpp"

#include "Core/UIManager.hpp"

#include "Graphics/Texture.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

#include "Model.hpp"

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace DirectX;

namespace helios
{
	void Model::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, std::wstring_view modelPath, gfx::Descriptor& srvCbDescriptor, std::wstring_view modelName, uint32_t textureIndex)
	{
		m_ModelName = modelName.data();

		std::string modelPathStr = WstringToString(modelPath);
		std::string modelDirectoryPathStr = modelPathStr.substr(0, modelPathStr.find_last_of(L'/') + 1);

		std::string warning{};
		std::string error{};

		tinygltf::TinyGLTF context{};

		tinygltf::Model model{};

		if (s_LoadedGLTFModels.find(modelPath.data()) != s_LoadedGLTFModels.end())
		{
			Model model = s_LoadedGLTFModels[modelPath.data()];

			m_PositionBuffer = model.GetPositionStructuredBuffer();
			std::wstring positionBufferName = m_ModelName + L" Position Buffer";
			m_PositionBuffer.GetResource()->SetName(positionBufferName.c_str());

			m_NormalBuffer = model.GetNormalStructuredBuffer();
			std::wstring normalBufferName = m_ModelName + L" Normal Buffer";
			m_NormalBuffer.GetResource()->SetName(normalBufferName.c_str());

			m_TextureCoordsBuffer = model.GetTextureStructuredBuffer();
			std::wstring textureBufferName = m_ModelName + L" Texture Buffer";
			m_TextureCoordsBuffer.GetResource()->SetName(textureBufferName.c_str());

			m_TangentBuffer = model.GetTangentStructuredBuffer();
			std::wstring tangentBufferName = m_ModelName + L" Tangent Buffer";
			m_TangentBuffer.GetResource()->SetName(tangentBufferName.c_str());

			m_IndicesCount = static_cast<uint32_t>(model.GetIndicesCount());
			
			m_IndexBuffer = model.GetIndexBuffer();

			m_TransformConstantBuffer.Init(device, commandList, TransformData{ .modelMatrix = dx::XMMatrixIdentity(), .inverseModelMatrix = dx::XMMatrixIdentity(), .viewMatrix = dx::XMMatrixIdentity(), .projectionMatrix = dx::XMMatrixIdentity() },
				srvCbDescriptor, m_ModelName + L" Transform CBuffer");

			m_TextureIndex = textureIndex;

			m_TransformCBufferIndexInDescriptorHeap = m_TransformConstantBuffer.GetBufferIndex();

			m_TransformData =
			{
				.rotation = dx::XMFLOAT3(0.0f, 0.0f, 0.0f),
				.scale = dx::XMFLOAT3(1.0f, 1.0f, 1.0f),
				.translate = dx::XMFLOAT3(0.0f, 0.0f, 0.0f)
			};

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

		std::vector<dx::XMFLOAT3> modelPositions{};
		std::vector<dx::XMFLOAT2> modelTextureCoords{};
		std::vector<dx::XMFLOAT3> modelNormals{};
		std::vector<dx::XMFLOAT4> modelTangents{};

		std::vector<uint32_t> indices{};

		std::vector<std::wstring> albedoTexturePaths{};
		std::vector<std::wstring> normalTexturePaths{};

		tinygltf::Scene& scene = model.scenes[model.defaultScene];

		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			tinygltf::Node& node = model.nodes[scene.nodes[i]];
			if (node.mesh < 0)
			{
				node.mesh = 0;
			}

			tinygltf::Mesh& nodeMesh = model.meshes[node.mesh];
			for (size_t i = 0; i < nodeMesh.primitives.size(); ++i)
			{
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
				for (size_t i = 0; i < positionAccesor.count; ++i)
				{
					dx::XMFLOAT3 position{};
					position.x = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[0];
					position.y = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[1];
					position.z = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[2];

					dx::XMFLOAT2 textureCoord{};
					textureCoord.x = (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[0];
					textureCoord.y = (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1];
					textureCoord.y = 1.0f - textureCoord.y;

					dx::XMFLOAT3 normal{};
					normal.x = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0];
					normal.y = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1];
					normal.z = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2];

					dx::XMFLOAT4 tangent{};
					tangent.x = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[0];
					tangent.y = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[1];
					tangent.z = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[2];
					tangent.w = (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[3];

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
				for (size_t i = 0; i < indexAccesor.count; ++i)
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

				if (gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
				{
					tinygltf::Texture& albedoTexture = model.textures[gltfPBRMaterial.pbrMetallicRoughness.baseColorTexture.index];
					tinygltf::Image& albedoImage = model.images[albedoTexture.source];

					albedoTexturePaths.push_back(StringToWString(modelDirectoryPathStr + albedoImage.uri));
				}

				if (gltfPBRMaterial.normalTexture.index >= 0)
				{
					tinygltf::Texture& normalTexture = model.textures[gltfPBRMaterial.normalTexture.index];
					tinygltf::Image& normalImage = model.images[normalTexture.source];

					normalTexturePaths.push_back(StringToWString(modelDirectoryPathStr + normalImage.uri));
				}
			}
		}

		m_PositionBuffer.Init(device, commandList, srvCbDescriptor, modelPositions, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Position Buffer");
		m_TextureCoordsBuffer.Init(device, commandList, srvCbDescriptor, modelTextureCoords, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Texture Coords Buffer");
		m_NormalBuffer.Init(device, commandList, srvCbDescriptor, modelNormals, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Normal Buffer");
		m_TangentBuffer.Init(device, commandList, srvCbDescriptor, modelTangents, D3D12_RESOURCE_FLAG_NONE, m_ModelName + L" Tanget Buffer");

		m_IndicesCount = static_cast<uint32_t>(indices.size());

		m_IndexBuffer.Init(device, commandList, indices, L"Index Buffer");
		m_TransformConstantBuffer.Init(device, commandList, TransformData{ .modelMatrix = dx::XMMatrixIdentity(), .inverseModelMatrix = dx::XMMatrixIdentity(), .viewMatrix = dx::XMMatrixIdentity(), .projectionMatrix = dx::XMMatrixIdentity()},
			srvCbDescriptor, m_ModelName + L" Transform CBuffer");

		m_TextureIndex = textureIndex;

		m_TransformCBufferIndexInDescriptorHeap = m_TransformConstantBuffer.GetBufferIndex();

		m_TransformData =
		{
			.rotation = dx::XMFLOAT3(0.0f, 0.0f, 0.0f),
			.scale = dx::XMFLOAT3(1.0f, 1.0f, 1.0f),
			.translate = dx::XMFLOAT3(0.0f, 0.0f, 0.0f)
		};

		s_LoadedGLTFModels[modelPath.data()] = *this;
	}
	
	void Model::UpdateData()
	{
		ImGui::Begin(WstringToString(m_ModelName).c_str());

		ImGui::SliderFloat("Scale", &m_TransformData.scale.x, 0.1f, 10.0f);
		m_TransformData.scale.y = m_TransformData.scale.x;
		m_TransformData.scale.z = m_TransformData.scale.x;

		ImGui::SliderFloat3("Translate", &m_TransformData.translate.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Rotate", &m_TransformData.rotation.x, -90.0f, 90.0f);

		ImGui::End();
	}

	void Model::UpdateTransformData(ID3D12GraphicsCommandList* commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix)
	{
		UpdateData();

		dx::XMVECTOR scalingVector = dx::XMLoadFloat3(&m_TransformData.scale);
		dx::XMVECTOR rotationVector = dx::XMLoadFloat3(&m_TransformData.rotation);
		dx::XMVECTOR translationVector = dx::XMLoadFloat3(&m_TransformData.translate);

		m_TransformConstantBuffer.GetBufferData().modelMatrix = dx::XMMatrixTranspose(dx::XMMatrixScalingFromVector(scalingVector) *  dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) * dx::XMMatrixTranslationFromVector(translationVector));
		m_TransformConstantBuffer.GetBufferData().inverseModelMatrix = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, m_TransformConstantBuffer.GetBufferData().modelMatrix));
		m_TransformConstantBuffer.GetBufferData().projectionMatrix = projectionMatrix;
		m_TransformConstantBuffer.GetBufferData().viewMatrix = viewMatrix;

		m_TransformConstantBuffer.Update();
	}

    void Model::Draw(ID3D12GraphicsCommandList* const commandList)
    {
		auto indexBufferView = m_IndexBuffer.GetBufferView();

		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(m_IndicesCount, 1u, 0u, 0u, 0u);
    }

}
