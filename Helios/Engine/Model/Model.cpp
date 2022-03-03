#include "Pch.hpp"

#include "Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION 
#define TINYOBJLOADER_USE_MAPBOX_EARCUT

#include "tiny_obj_loader.h"

namespace helios
{
	void Model::Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath)
	{
        tinyobj::ObjReaderConfig readerConfig{};
        readerConfig.mtl_search_path = "./"; 

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(WstringToString(modelPath), readerConfig)) 
        {
            if (!reader.Error().empty()) 
            {
                OutputDebugStringA(reader.Error().c_str());
            }
            exit(1);
        }

        if (!reader.Warning().empty()) 
        {
            OutputDebugStringA(reader.Warning().c_str());
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        std::vector<Vertex> vertices{};

        for (size_t s = 0; s < shapes.size(); s++) 
        {
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
            {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                for (size_t v = 0; v < fv; v++) 
                {
                    Vertex vertex{};

                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
              
                    vertex =
                    {
                        .position = dx::XMFLOAT3(attrib.vertices[3 * size_t(idx.vertex_index) + 0], 
                                                 attrib.vertices[3 * size_t(idx.vertex_index) + 1], 
                                                 attrib.vertices[3 * size_t(idx.vertex_index) + 2]),

                        .normal = dx::XMFLOAT3(attrib.normals[3 * size_t(idx.normal_index) + 0], 
                                               attrib.normals[3 * size_t(idx.normal_index) + 1], 
                                               attrib.normals[3 * size_t(idx.normal_index) + 2]),

                        .textureCoord = dx::XMFLOAT2(attrib.texcoords[2 * size_t(idx.texcoord_index) + 0], 
                                                     attrib.texcoords[2 * size_t(idx.texcoord_index) + 1])
                    };

                    vertices.push_back(std::move(vertex));
                }

                index_offset += fv;
            }
        }

        m_VertexBuffer.Init<Vertex>(device, commandList, vertices);

        //5vertices.clear();
	}

    D3D12_VERTEX_BUFFER_VIEW Model::GetVertexBufferView()
    {
        return m_VertexBuffer.GetBufferView();
    }
}
