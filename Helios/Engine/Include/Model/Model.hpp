#pragma once

#include "Pch.hpp"

#include "Include/Graphics/VertexBuffer.hpp"

namespace helios
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position{};
		DirectX::XMFLOAT3 normal{};
		DirectX::XMFLOAT2 textureCoord{};
	};

	struct Transform
	{
		DirectX::XMMATRIX modelMatrix;
	};

	// This class is temporary. In the future, a GLTF loader will be created which will be mainly used for model loading.
	// Temporarily using tinyobjloader and the .obj format.
	class Model
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

	private:
		gfx::VertexBuffer m_VertexBuffer{};
	};
}

