#include "Pch.hpp"

#include "Model.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

namespace helios
{
	void Model::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath)
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

		tinygltf::Scene const& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			tinygltf::Node const& node = model.nodes[scene.nodes[i]]; 
			if (node.mesh < 0 || node.mesh >= model.meshes.size()) continue;

			tinygltf::Mesh const& node_mesh = model.meshes[node.mesh];
			for (size_t i = 0; i < node_mesh.primitives.size(); ++i)
			{
				tinygltf::Primitive primitive = node_mesh.primitives[i];
				tinygltf::Accessor const& index_accessor = model.accessors[primitive.indices];


				tinygltf::Accessor const& position_accessor = model.accessors[primitive.attributes["POSITION"]];
				tinygltf::BufferView const& position_buffer_view = model.bufferViews[position_accessor.bufferView];
				tinygltf::Buffer const& position_buffer = model.buffers[position_buffer_view.buffer];
				int const position_byte_stride = position_accessor.ByteStride(position_buffer_view);
				uint8_t const* positions = &position_buffer.data[position_buffer_view.byteOffset + position_accessor.byteOffset];

				tinygltf::Accessor const& texcoord_accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
				tinygltf::BufferView const& texcoord_buffer_view = model.bufferViews[texcoord_accessor.bufferView];
				tinygltf::Buffer const& texcoord_buffer = model.buffers[texcoord_buffer_view.buffer];
				int const texcoord_byte_stride = texcoord_accessor.ByteStride(texcoord_buffer_view);
				uint8_t  const* texcoords = &texcoord_buffer.data[texcoord_buffer_view.byteOffset + texcoord_accessor.byteOffset];

				tinygltf::Accessor const& normal_accessor = model.accessors[primitive.attributes["NORMAL"]];
				tinygltf::BufferView const& normal_buffer_view = model.bufferViews[normal_accessor.bufferView];
				tinygltf::Buffer const& normal_buffer = model.buffers[normal_buffer_view.buffer];
				int const normal_byte_stride = normal_accessor.ByteStride(normal_buffer_view);
				uint8_t  const* normals = &normal_buffer.data[normal_buffer_view.byteOffset + normal_accessor.byteOffset];

				tinygltf::Accessor const& tangent_accessor = model.accessors[primitive.attributes["TANGENT"]];
				tinygltf::BufferView const& tangent_buffer_view = model.bufferViews[tangent_accessor.bufferView];
				tinygltf::Buffer const& tangent_buffer = model.buffers[tangent_buffer_view.buffer];
				int const tangent_byte_stride = tangent_accessor.ByteStride(tangent_buffer_view);
				uint8_t  const* tangents = &tangent_buffer.data[tangent_buffer_view.byteOffset + tangent_accessor.byteOffset];

				for (size_t i = 0; i < position_accessor.count; ++i)
				{
					Vertex vertex{};

					vertex.position.x = (reinterpret_cast<float const*>(positions + (i * position_byte_stride)))[0];
					vertex.position.y = (reinterpret_cast<float const*>(positions + (i * position_byte_stride)))[1];
					vertex.position.z = (reinterpret_cast<float const*>(positions + (i * position_byte_stride)))[2];

					vertex.textureCoord.x = (reinterpret_cast<float const*>(texcoords + (i * texcoord_byte_stride)))[0];
					vertex.textureCoord.y = (reinterpret_cast<float const*>(texcoords + (i * texcoord_byte_stride)))[1];
					vertex.textureCoord.y = 1.0f - vertex.textureCoord.y;

					vertex.normal.x = (reinterpret_cast<float const*>(normals + (i * normal_byte_stride)))[0];
					vertex.normal.y = (reinterpret_cast<float const*>(normals + (i * normal_byte_stride)))[1];
					vertex.normal.z = (reinterpret_cast<float const*>(normals + (i * normal_byte_stride)))[2];

					vertices.push_back(vertex);
				}

				tinygltf::BufferView const& index_buffer_view = model.bufferViews[index_accessor.bufferView];
				tinygltf::Buffer const& index_buffer = model.buffers[index_buffer_view.buffer];
				int const index_byte_stride = index_accessor.ByteStride(index_buffer_view);
				uint8_t const* indexes = index_buffer.data.data() + index_buffer_view.byteOffset + index_accessor.byteOffset;

				for (size_t i = 0; i < index_accessor.count; ++i)
				{
					uint32_t index = (uint32_t)(reinterpret_cast<uint16_t const*>(indexes + (i * index_byte_stride)))[0];
	
					m_Indices.push_back(index);
				}
			}
		}

		m_VerticesCount = vertices.size();
		m_IndicesCount = m_Indices.size();

		m_VertexBuffer.Init<Vertex>(device, commandList, vertices);
		m_IndexBuffer.Init(device, commandList, m_Indices);
	}

    D3D12_VERTEX_BUFFER_VIEW Model::GetVertexBufferView()
    {
        return m_VertexBuffer.GetBufferView();
    }

    void Model::Draw(ID3D12GraphicsCommandList* commandList)
    {
		auto indexBufferView = m_IndexBuffer.GetBufferView();
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(m_IndicesCount, 1u, 0u, 0u, 0u);
    }

}
