#pragma once

#include "Pch.hpp"

#include "Graphics/VertexBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include "Graphics/ConstantBuffer.hpp"

namespace helios
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position{ DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f) };
		DirectX::XMFLOAT3 normal{ DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f) };
		DirectX::XMFLOAT2 textureCoord{ DirectX::XMFLOAT2(0.0f, 0.0f) };
	};

	struct alignas(256) Transform
	{
		DirectX::XMMATRIX modelMatrix{DirectX::XMMatrixIdentity()};
		DirectX::XMMATRIX inverseModelMatrix{ DirectX::XMMatrixIdentity() };
		DirectX::XMMATRIX projectionViewMatrix{ DirectX::XMMatrixIdentity() };
	};

	struct TransformComponent
	{
		DirectX::XMFLOAT3 rotation{DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)};
		DirectX::XMFLOAT3 scale{ DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f) };
		DirectX::XMFLOAT3 translate{ DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f) };
	};

	class Model
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath, D3D12_CPU_DESCRIPTOR_HANDLE cbCPUDescriptorHandle);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
		D3D12_GPU_VIRTUAL_ADDRESS GetTransformCBufferVirtualAddress();

		TransformComponent& GetTransform();
		void UpdateData(std::wstring_view objectName);

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

