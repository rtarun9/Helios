#include "Pch.hpp"

#include "PointLight.hpp"

#include "imgui.h"

namespace helios::gfx
{
	void PointLight::InitLightDataCBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor)
	{
		s_PointLightData.Init(device, commandList, PointLightData{}, srvCbDescriptor, L"Point Light CBuffer");
	}

	void PointLight::Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor, float radius, DirectX::XMFLOAT3 color, DirectX::XMFLOAT4 position, uint32_t lightNumber)
	{
		m_LightNumber = lightNumber;
		std::wstring modelName = L"Point Light " + std::to_wstring(m_LightNumber);
		std::wstring lightDataCBufferName = modelName + L" Light CBuffer";

		Model::Init(device, commandList, L"Assets/Models/Sphere/scene.gltf", srvCbDescriptor, modelName);

		s_PointLightData.GetBufferData().radius[m_LightNumber] = radius;
		s_PointLightData.GetBufferData().lightColor[m_LightNumber] = DirectX::XMFLOAT4{color.x, color.y, color.z, 1.0f};
		s_PointLightData.GetBufferData().lightPosition[m_LightNumber] = position;

		m_TransformData.translate = { position.x, position.y, position.z };
		m_TransformData.scale = { radius, radius, radius };
	}

	void PointLight::UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix)
	{
		UpdateData();

		s_PointLightData.Update();

		dx::XMVECTOR scalingVector = dx::XMLoadFloat3(&m_TransformData.scale);
		dx::XMVECTOR rotationVector = dx::XMLoadFloat3(&m_TransformData.rotation);
		dx::XMVECTOR translationVector = dx::XMLoadFloat3(&m_TransformData.translate);

		m_TransformConstantBuffer.GetBufferData().modelMatrix = dx::XMMatrixTranspose(dx::XMMatrixScalingFromVector(scalingVector) * dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) * dx::XMMatrixTranslationFromVector(translationVector));
		m_TransformConstantBuffer.GetBufferData().inverseModelMatrix = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, m_TransformConstantBuffer.GetBufferData().modelMatrix));
		m_TransformConstantBuffer.GetBufferData().projectionMatrix = projectionMatrix;
		m_TransformConstantBuffer.GetBufferData().viewMatrix = viewMatrix;

		m_TransformConstantBuffer.Update();
	}

	void PointLight::UpdateData()
	{
		if (ImGui::TreeNode(WstringToString(m_ModelName).c_str()))
		{
			ImGui::SliderFloat("Radius", &m_TransformData.scale.x, 0.0f, 10.0f);
			m_TransformData.scale.y = m_TransformData.scale.x;
			m_TransformData.scale.z = m_TransformData.scale.x;
			s_PointLightData.GetBufferData().radius[m_LightNumber] = m_TransformData.scale.x;

			ImGui::ColorPicker3("Color", &s_PointLightData.GetBufferData().lightColor[m_LightNumber].x);

			ImGui::SliderFloat3("Translate", &m_TransformData.translate.x, -50.0f, 50.0f);
			
			s_PointLightData.GetBufferData().lightPosition[m_LightNumber] = { m_TransformData.translate.x, m_TransformData.translate.y, m_TransformData.translate.z, 0.0f };

			ImGui::SliderFloat3("Rotate", &m_TransformData.rotation.x, -90.0f, 90.0f);

			ImGui::TreePop();
		}
	}
}