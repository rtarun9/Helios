#include "Pch.hpp"

#include "DirectionalLight.hpp"

#include "imgui.h"

namespace helios::gfx
{
	void DirectionalLight::InitLightDataCBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor)
	{
		s_DirectionalLightData.Init(device, commandList, DirectionalLightData{}, srvCbDescriptor, L"Point Light CBuffer");
	}

	void DirectionalLight::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor, float sunAngle, DirectX::XMFLOAT4 lightColor, uint32_t lightNumber)
	{
		m_LightNumber = lightNumber;

		s_DirectionalLightData.GetBufferData().lightColor[m_LightNumber] = lightColor;
		s_DirectionalLightData.GetBufferData().lightDirection[m_LightNumber] = dx::XMFLOAT4(0.0f, sin(dx::XMConvertToRadians(sunAngle)), cos(dx::XMConvertToRadians(sunAngle)), 0.0f);
		m_SunAngle = sunAngle;

		s_DirectionalLightData.Update();
	}

	void DirectionalLight::UpdateData()
	{
		static std::string lightName = std::string("Directional Light ") + std::to_string(m_LightNumber);

		if (ImGui::TreeNode(lightName.c_str()))
		{
			ImGui::SliderFloat("Sun angle", &m_SunAngle, -180.0f, 180.0f);

			ImGui::ColorPicker3("Color", &s_DirectionalLightData.GetBufferData().lightColor[m_LightNumber].x);

			ImGui::TreePop();
		}

		s_DirectionalLightData.GetBufferData().lightDirection[m_LightNumber] = dx::XMFLOAT4(0.0f, sin(dx::XMConvertToRadians(m_SunAngle)), cos(dx::XMConvertToRadians(m_SunAngle)), 0.0f);
		s_DirectionalLightData.Update();
	}
}