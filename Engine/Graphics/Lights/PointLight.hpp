#pragma once

#include "Pch.hpp"

#include "Graphics/API/ConstantBuffer.hpp"

#include "Graphics/Model/Model.hpp"

namespace helios::gfx
{
	class PointLight : public Model
	{
	public:
		static void InitLightDataCBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor);
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor, float radius, DirectX::XMFLOAT3 color, DirectX::XMFLOAT4 position, uint32_t lightNumber);

		void UpdateData();
		void UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix);

		uint32_t GetPointLightIndex() const { return m_LightNumber; };

		static PointLightData& GetPointLightData() { return s_PointLightData.GetBufferData(); }
		static uint32_t GetLightDataCBufferIndex() { return s_PointLightData.GetBufferIndex(); }

	private:
		uint32_t m_LightNumber{};

		static inline gfx::ConstantBuffer<PointLightData> s_PointLightData{};
	};
}