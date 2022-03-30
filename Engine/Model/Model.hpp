#pragma once

#include "Pch.hpp"

#include "Graphics/StructuredBuffer.hpp"
#include "Graphics/IndexBuffer.hpp"
#include "Graphics/ConstantBuffer.hpp"

#include "Graphics/Descriptor.hpp"

namespace helios
{
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

	// Note : This is not the final design for materials since it is currently only used for non - PBR materials.
	struct Texture
	{
		D3D12_GPU_DESCRIPTOR_HANDLE m_BaseColorDescriptorHandle{};
	};

	class Model
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring_view modelPath, gfx::Descriptor& cbDescriptor, Texture material = {});

		ID3D12Resource* GetPositionBuffer();
		ID3D12Resource* GetTextureCoordsBuffer();
		ID3D12Resource* GetNormalBuffer();

		D3D12_GPU_VIRTUAL_ADDRESS GetTransformCBufferVirtualAddress();
		Texture GetTexture();

		TransformComponent& GetTransform();
		void UpdateData(std::wstring_view objectName);

		void UpdateTransformData(ID3D12GraphicsCommandList* commandList, DirectX::XMMATRIX projectionViewMatrix);

		void Draw(ID3D12GraphicsCommandList* commandList);

	private:
		
		gfx::StructuredBuffer m_PositionBuffer{};
		gfx::StructuredBuffer m_TextureCoordsBuffer{};
		gfx::StructuredBuffer m_NormalBuffer{};

		gfx::IndexBuffer m_IndexBuffer{};
		gfx::ConstantBuffer<Transform> m_TransformConstantBuffer{};

		uint32_t m_IndicesCount{};

		TransformComponent m_TransformData{};

		// Note : Currently only used for non - PBR Models.
		Texture m_Texture{};
	};
}

