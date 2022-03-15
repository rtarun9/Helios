#include "Pch.hpp"

#include "Model.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

// Some operator overloads are in the namespace, hence declaring it in global namespace here.
using namespace DirectX;

namespace helios
{
	void Model::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath, D3D12_CPU_DESCRIPTOR_HANDLE cbCPUDescriptorHandle)
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
                OutputDebugStringA(error.c_str());
            }

            if (!warning.empty())
            {
                OutputDebugStringA(warning.c_str());
            }
        }

        // Build meshes.

        std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		tinygltf::Scene& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			tinygltf::Node& node = model.nodes[scene.nodes[i]]; 
			if (node.mesh < 0 || node.mesh >= model.meshes.size())
			{
				continue;
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

				// Tangent and BITangent data (commented out for now).
				tinygltf::Accessor& tangentAccesor = model.accessors[primitive.attributes["TANGENT"]];
				tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccesor.bufferView];
				tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];
				int tangentByteStride = tangentAccesor.ByteStride(tangentBufferView);
				uint8_t  const* tangents = &tangentBuffer.data[tangentBufferView.byteOffset + tangentAccesor.byteOffset];

				// Fill in the vertices array.
				for (size_t i = 0; i < positionAccesor.count; ++i)
				{
					Vertex vertex{};

					vertex.position.x = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[0];
					vertex.position.y = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[1];
					vertex.position.z = (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[2];

					vertex.textureCoord.x = (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[0];
					vertex.textureCoord.y = (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1];
					vertex.textureCoord.y = 1.0f - vertex.textureCoord.y;

					vertex.normal.x = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0];
					vertex.normal.y = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1];
					vertex.normal.z = (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2];

					vertices.push_back(vertex);
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

		m_VerticesCount = static_cast<uint32_t>(vertices.size());
		m_IndicesCount = static_cast<uint32_t>(indices.size());

		m_VertexBuffer.Init<Vertex>(device, commandList, vertices);
		m_IndexBuffer.Init(device, commandList, indices);
		m_TransformConstantBuffer.Init(device, commandList, Transform{ .modelMatrix = dx::XMMatrixIdentity(), .inverseModelMatrix = dx::XMMatrixIdentity(), .projectionViewMatrix = dx::XMMatrixIdentity() },
			cbCPUDescriptorHandle);
	}

    D3D12_VERTEX_BUFFER_VIEW Model::GetVertexBufferView()
    {
        return m_VertexBuffer.GetBufferView();
    }

	D3D12_GPU_VIRTUAL_ADDRESS Model::GetTransformCBufferVirtualAddress()
	{
		auto bufferView = m_TransformConstantBuffer.GetBufferView();
		return bufferView.BufferLocation;
	}

	TransformComponent& Model::GetTransform()
	{
		return m_TransformData;
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
		auto vertexBufferView = m_VertexBuffer.GetBufferView();
		auto indexBufferView = m_IndexBuffer.GetBufferView();

		commandList->IASetVertexBuffers(0u, 1u, &vertexBufferView);
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(m_IndicesCount, 1u, 0u, 0u, 0u);
    }

}
