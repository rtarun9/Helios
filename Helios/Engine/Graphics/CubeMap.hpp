#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"
#include "Model/Model.hpp"

namespace helios::gfx
{
	class CubeMap
	{
	public:
		void Init(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Descriptor& srvDescriptor, Descriptor& rtvDescriptor, std::wstring_view path, std::wstring_view textureName);

		void DrawToCube(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,  Model& cubeModel);

		ID3D12Resource* GetTextureResource();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle();

	private:
		void* m_TextureData{};
		int m_Width{};
		int m_Height{};
		int m_ComponentCount{};

		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureUploadHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUDescriptorHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptorHandle{};
		
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> m_CubeFaceCPUDescriptorHandle{};
		std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 6> m_CubeFaceGPUDescriptorHandle{};

		std::array<DirectX::XMMATRIX, 6> m_LookAtMatrices{};
		D3D12_VIEWPORT m_Viewport{};
		DirectX::XMMATRIX m_ProjectionMatrix{};
	};
}

