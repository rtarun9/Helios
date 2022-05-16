#pragma once

#include "Pch.hpp"

#include "Graphics/API/ConstantBuffer.hpp"

#include "ConstantBuffers.hlsli"

namespace helios::gfx
{
	class DirectionalLight
	{
	public:
		static void InitLightDataCBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor);
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor, float sunAngle, DirectX::XMFLOAT4 lightColor, uint32_t lightNumber);

		void UpdateData();

		uint32_t GetDirectionalLightIndex() const { return m_LightNumber; };

		static DirectionalLightData& GetDirectionalLightData() { return s_DirectionalLightData.GetBufferData(); }
		static uint32_t GetLightDataCBufferIndex() { return s_DirectionalLightData.GetBufferIndex(); }

	private:
		uint32_t m_LightNumber{};
		float m_SunAngle{};
		static inline gfx::ConstantBuffer<DirectionalLightData> s_DirectionalLightData{};
	};
}