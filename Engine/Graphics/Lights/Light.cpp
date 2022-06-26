#include "Pch.hpp"

#include "Light.hpp"

#include "imgui.h"

/*
namespace helios::gfx
{
	void Light::InitLightDataCBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor)
	{
		s_LightData.Init(device, commandList, LightData{}, srvCbDescriptor, L"Light Data CBuffer");
	}

	void Light::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor, std::variant<PointLightData, DirectionalLightData> lightData, uint32_t lightNumber)
	{
		m_LightType = static_cast<uint32_t>(lightData.index());
		m_LightNumber = lightNumber;
		m_LightData = lightData;

		switch (m_LightType)
		{
			case EnumClassValue(LightTypes::PointLightData) : 
			{
				PointLightData data = std::get<PointLightData>(lightData);

				std::wstring modelName = L"Point Light " + std::to_wstring(m_LightNumber);

				Model::Init(device, commandList, SPHERE_MODEL_PATH, srvCbDescriptor, modelName);

				s_LightData.GetBufferData().radius[m_LightNumber] = data.radius;
				s_LightData.GetBufferData().lightColor[m_LightNumber] = data.lightColor;
				s_LightData.GetBufferData().lightPosition[m_LightNumber] = data.lightPosition;

				m_TransformData.translate = { data.lightPosition.x, data.lightPosition.y, data.lightPosition.z };
				m_TransformData.scale = { data.radius, data.radius, data.radius };

				s_LightData.Update();
			}
			break;

			case EnumClassValue(LightTypes::DirectionalLightData):
			{
				DirectionalLightData data = std::get<DirectionalLightData>(lightData);

				s_LightData.GetBufferData().lightColor[m_LightNumber] = data.lightColor;
				s_LightData.GetBufferData().lightPosition[m_LightNumber] = dx::XMFLOAT4(0.0f, sin(dx::XMConvertToRadians(data.sunAngle)), cos(dx::XMConvertToRadians(data.sunAngle)), 0.0f);
				s_LightData.GetBufferData().radius[m_LightNumber] = 0.0f;

				s_LightData.Update();
			}
			break;
		}
	}

	void Light::UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix)
	{
		UpdateData();

		if (m_LightType == EnumClassValue(LightTypes::PointLightData))
		{
			dx::XMVECTOR scalingVector = dx::XMLoadFloat3(&m_TransformData.scale);
			dx::XMVECTOR rotationVector = dx::XMLoadFloat3(&m_TransformData.rotation);
			dx::XMVECTOR translationVector = dx::XMLoadFloat3(&m_TransformData.translate);

			m_TransformConstantBuffer.GetBufferData().modelMatrix = dx::XMMatrixTranspose(dx::XMMatrixScalingFromVector(scalingVector) * dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) * dx::XMMatrixTranslationFromVector(translationVector));
			m_TransformConstantBuffer.GetBufferData().inverseModelMatrix = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, m_TransformConstantBuffer.GetBufferData().modelMatrix));
			m_TransformConstantBuffer.GetBufferData().projectionMatrix = projectionMatrix;
			m_TransformConstantBuffer.GetBufferData().viewMatrix = viewMatrix;

			m_TransformConstantBuffer.Update();
		}
	}

	void Light::UpdateData()
	{
		switch (m_LightType)
		{
			case EnumClassValue(LightTypes::PointLightData):
			{
				if (ImGui::TreeNode(WstringToString(m_ModelName).c_str()))
				{
					ImGui::SliderFloat("Radius", &m_TransformData.scale.x, 0.0f, 10.0f);
					m_TransformData.scale.y = m_TransformData.scale.x;
					m_TransformData.scale.z = m_TransformData.scale.x;
					
					s_LightData.GetBufferData().radius[m_LightNumber] = m_TransformData.scale.x;

					ImGui::ColorEdit3("Color", &s_LightData.GetBufferData().lightColor[m_LightNumber].x);

					ImGui::SliderFloat3("Translate", &m_TransformData.translate.x, -50.0f, 50.0f);

					s_LightData.GetBufferData().lightPosition[m_LightNumber] = { m_TransformData.translate.x, m_TransformData.translate.y, m_TransformData.translate.z, 1.0f };

					ImGui::SliderFloat3("Rotate", &m_TransformData.rotation.x, -90.0f, 90.0f);

					ImGui::TreePop();
				}
			}
			break;

			case EnumClassValue(LightTypes::DirectionalLightData):
			{
				static std::string lightName = std::string("Directional Light ") + std::to_string(m_LightNumber);
				float& sunAngle = std::get<DirectionalLightData>(m_LightData).sunAngle;

				if (ImGui::TreeNode(lightName.c_str()))
				{
					ImGui::SliderFloat("Sun angle", &sunAngle, -180.0f, 180.0f);

					ImGui::ColorPicker3("Color", &s_LightData.GetBufferData().lightColor[m_LightNumber].x);

					ImGui::TreePop();
				}

				s_LightData.GetBufferData().lightPosition[m_LightNumber] = dx::XMFLOAT4(0.0f, sin(dx::XMConvertToRadians(sunAngle)), cos(dx::XMConvertToRadians(sunAngle)), 0.0f);
			}
			break;
		}

		s_LightData.Update();
	}
}
*/