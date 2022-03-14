#pragma once

#include "Pch.hpp"

#include "Graphics/VertexBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include "Graphics/ConstantBuffer.hpp"

namespace helios
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position{};
		DirectX::XMFLOAT3 normal{};
		DirectX::XMFLOAT2 textureCoord{};
	};

	struct alignas(256) Transform
	{
		DirectX::XMMATRIX modelMatrix{};
		DirectX::XMMATRIX inverseModelMatrix{};
		DirectX::XMMATRIX projectionViewMatrix{};
	};

	struct TransformComponent
	{
		DirectX::XMFLOAT3 rotation{};
		DirectX::XMFLOAT3 scale{};
		DirectX::XMFLOAT3 translate{};
	};

	class Model
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath, D3D12_CPU_DESCRIPTOR_HANDLE cbCPUDescriptorHandle);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
		D3D12_GPU_VIRTUAL_ADDRESS GetTransformCBufferVirtualAddress();

		TransformComponent& GetTransform();

		void UpdateTransformData(ID3D12GraphicsCommandList* commandList, DirectX::XMMATRIX projectionViewMatrix);

		void Draw(ID3D12GraphicsCommandList* commandList);

	private:
		gfx::VertexBuffer m_VertexBuffer{};
		gfx::IndexBuffer m_IndexBuffer{};
		gfx::ConstantBuffer<Transform> m_TransformConstantBuffer{};

		uint32_t m_VerticesCount{};
		uint32_t m_IndicesCount{};

		TransformComponent m_TransformData{};
	};
}

