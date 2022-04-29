#include "Pch.hpp"

#include "Model.hpp"
#include "Core/UIManager.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace DirectX;

namespace helios
{
	void Model::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath, gfx::Descriptor& srvCbDescriptor, uint32_t textureIndex)
	{
        auto modelPathStr = WstringToString(modelPath);

        std::string warning{};
        std::string error{};

        tinygltf::TinyGLTF context{};

		tinygltf::Model model{};

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
		std::vector<dx::XMFLOAT3> modelBiTangents{};
		std::vector<dx::XMFLOAT4> modelTangents{};

		std::vector<uint32_t> indices{};

		tinygltf::Scene& scene = model.scenes[model.defaultScene];

		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			tinygltf::Node& node = model.nodes[scene.nodes[i]]; 
			if (node.mesh < 0)
			{
				node.mesh = 0;
			}

			tinygltf::Mesh& node_mesh = model.meshes[node.mesh];
			for (size_t i = 0; i < node_mesh.primitives.size(); ++i)
			{
				// Get Accesor, buffer view and buffer for each attribute (position, textureCoord, normal).
				tinygltf::Primitive primitive = node_mesh.primitives[i];
				tinygltf::Accessor& indexAccesor = model.accessors[primitive.indices];

				// Position data.
				tinygltf::Accessor& positionAccesor = model.accessors[primitive.attributes["POSITION"]];
				tinygltf::BufferView& positionBufferView= model.bufferViews[positionAccesor.bufferView];
				tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
				
				int positionByteStride = positionAccesor.ByteStride(positionBufferView);
				uint8_t* positions = &positionBuffer.data[positionBufferView.byteOffset + positionAccesor.byteOffset];

				// TextureCoord data.
				tinygltf::Accessor& textureCoordAccesor = model.accessors[primitive.attributes["TEXCOORD_0"]];
				tinygltf::BufferView& textureCoordBufferView = model.bufferViews[textureCoordAccesor.bufferView];
				tinygltf::Buffer& textureCoordBuffer = model.buffers[textureCoordBufferView.buffer];
				int textureCoordBufferStride = textureCoordAccesor.ByteStride(textureCoordBufferView);
				uint8_t* texcoords = &textureCoordBuffer.data[textureCoordBufferView.byteOffset + textureCoordAccesor.byteOffset];

				// Normal data.
				tinygltf::Accessor& normalAccesor = model.accessors[primitive.attributes["NORMAL"]];
				tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccesor.bufferView];
				tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
				int normalByteStride = normalAccesor.ByteStride(normalBufferView);
				uint8_t* normals = &normalBuffer.data[normalBufferView.byteOffset + normalAccesor.byteOffset];

				// Tangent data.
				tinygltf::Accessor& tangentAccesor = model.accessors[primitive.attributes["TANGENT"]];
				tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccesor.bufferView];
				tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
				int tangentByteStride = tangentAccesor.ByteStride(tangentBufferView);
				uint8_t  const* tangents = &tangentBuffer.data[tangentBufferView.byteOffset + tangentAccesor.byteOffset];

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

					dx::XMFLOAT3 tangentCoord{ tangent.x, tangent.y, tangent.z };
					dx::XMFLOAT3 biTangent{};
					XMVECTOR biTangentVec = XMVectorScale(XMVector3Cross(XMLoadFloat3(&normal), XMLoadFloat3(&tangentCoord)), tangent.w);

					XMStoreFloat3(&biTangent, XMVector3Normalize(biTangentVec));

					modelPositions.emplace_back(position);
					modelTextureCoords.emplace_back(textureCoord);
					modelNormals.emplace_back(normal);
					modelBiTangents.emplace_back(biTangent);
					modelTangents.emplace_back(tangent);
				}

				// Get the index buffer data.
				tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccesor.bufferView];
				tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
				int indexByteStride = indexAccesor.ByteStride(indexBufferView);
				uint8_t* indexes = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccesor.byteOffset;

				// Fill indices array.
				for (size_t i = 0; i < indexAccesor.count; ++i)
				{
					uint32_t index = (uint32_t)(reinterpret_cast<uint16_t const*>(indexes + (i * indexByteStride)))[0];
	
					indices.push_back(index);
				}
			}
		}

		m_PositionBuffer.Init<dx::XMFLOAT3>(device, commandList, srvCbDescriptor, modelPositions, D3D12_RESOURCE_FLAG_NONE, L"Position Buffer");
		m_TextureCoordsBuffer.Init<dx::XMFLOAT2>(device, commandList, srvCbDescriptor, modelTextureCoords, D3D12_RESOURCE_FLAG_NONE, L"Texture Coords Buffer");
		m_NormalBuffer.Init<dx::XMFLOAT3>(device, commandList, srvCbDescriptor, modelNormals, D3D12_RESOURCE_FLAG_NONE, L"Normal Buffer");
		m_BitangentBuffer.Init<dx::XMFLOAT3>(device, commandList, srvCbDescriptor, modelBiTangents, D3D12_RESOURCE_FLAG_NONE, L"Bi Tanget Buffer");
		m_TangentBuffer.Init<dx::XMFLOAT4>(device, commandList, srvCbDescriptor, modelTangents, D3D12_RESOURCE_FLAG_NONE, L"Tanget Buffer");

		m_IndicesCount = static_cast<uint32_t>(indices.size());

		m_IndexBuffer.Init(device, commandList, indices, L"Index Buffer");
		m_TransformConstantBuffer.Init(device, commandList, Transform{ .modelMatrix = dx::XMMatrixIdentity(), .inverseModelMatrix = dx::XMMatrixIdentity(), .projectionViewMatrix = dx::XMMatrixIdentity() },
			srvCbDescriptor, L"Transform CBuffer");

		m_TextureIndex = textureIndex;

		m_TransformCBufferIndexInDescriptorHeap = m_TransformConstantBuffer.GetBufferIndex();
	}

	ID3D12Resource* Model::GetPositionBuffer() const
	{
		return m_PositionBuffer.GetResource();
	}

	ID3D12Resource* Model::GetTextureCoordsBuffer() const
	{
		return m_TextureCoordsBuffer.GetResource();
	}

	ID3D12Resource* Model::GetNormalBuffer() const
	{
		return m_NormalBuffer.GetResource();
	}

	ID3D12Resource* Model::GetBiTangetBuffer() const
	{
		return m_BitangentBuffer.GetResource();
	}

	ID3D12Resource* Model::GetTangetBuffer() const
	{
		return m_TangentBuffer.GetResource();
	}

	uint32_t Model::GetPositionBufferIndex() const
	{
		return m_PositionBuffer.GetSRVIndex();
	}

	uint32_t Model::GetTextureCoordsBufferIndex() const
	{
		return m_TextureCoordsBuffer.GetSRVIndex();
	}

	uint32_t Model::GetNormalBufferIndex() const
	{
		return m_NormalBuffer.GetSRVIndex();
	}

	uint32_t Model::GetBiTangentBufferIndex() const
	{
		return m_BitangentBuffer.GetSRVIndex();
	}

	uint32_t Model::GetTangentBufferIndex() const
	{
		return m_TangentBuffer.GetSRVIndex();
	}

	uint32_t Model:: GetTransformCBufferIndex() const
	{
		return m_TransformCBufferIndexInDescriptorHeap;
	}

	uint32_t Model::GetTextureIndex() const
	{
		return m_TextureIndex;
	}

	TransformComponent& Model::GetTransform()
	{
		return m_TransformData;
	}
	
	void Model::UpdateData(std::wstring_view objectName)
	{
		ImGui::Begin(WstringToString(objectName.data()).c_str());

		ImGui::SliderFloat3("Translate", &m_TransformData.translate.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Rotate", &m_TransformData.rotation.x, -90.0f, 90.0f);

		ImGui::End();
	}

	void Model::UpdateTransformData(ID3D12GraphicsCommandList* commandList, DirectX::XMMATRIX projectionViewMatrix)
	{
		auto scalingVector = dx::XMLoadFloat3(&m_TransformData.scale);
		auto rotationVector = dx::XMLoadFloat3(&m_TransformData.rotation);
		auto translationVector = dx::XMLoadFloat3(&m_TransformData.translate);

		m_TransformConstantBuffer.GetBufferData().modelMatrix = dx::XMMatrixScalingFromVector(scalingVector) *  dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) * dx::XMMatrixTranslationFromVector(translationVector);
		m_TransformConstantBuffer.GetBufferData().inverseModelMatrix = dx::XMMatrixInverse(nullptr, m_TransformConstantBuffer.GetBufferData().modelMatrix);
		m_TransformConstantBuffer.GetBufferData().projectionViewMatrix = projectionViewMatrix;

		m_TransformConstantBuffer.Update();
	}

    void Model::Draw(ID3D12GraphicsCommandList* commandList)
    {
		auto indexBufferView = m_IndexBuffer.GetBufferView();

		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(m_IndicesCount, 1u, 0u, 0u, 0u);
    }

}
