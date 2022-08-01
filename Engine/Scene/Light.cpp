#include "Pch.hpp"

#include "Light.hpp"

namespace helios::scene
{
	void Light::CreateLightDataBuffer(const gfx::Device* device)
	{
		gfx::BufferCreationDesc lightDataBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Light Constant Buffer",
		};

		sLightBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<LightBuffer>(lightDataBufferCreationDesc, std::span<const LightBuffer, 0u>{}));
		sLightBufferData = {};
	}

	void Light::DestroyLightDataBuffer()
	{
		sLightBuffer->allocation->Reset();
	}

	Light::Light(const LightCreationDesc& lightCreationDesc) : mLightNumber(lightCreationDesc.lightNumber), mLightType(lightCreationDesc.lightType)
	{
		// The value 0.001 is hardcoded, which is not ideal.
		// The sphere model is very large, which is why this happens.
		sLightBufferData.lightColor[mLightNumber] = { 1.0f, 1.0f, 1.0f, 1.0f };
		sLightBufferData.radius[mLightNumber] = 0.001f;

		if (lightCreationDesc.lightType == LightTypes::DirectionalLightData)
		{
			sLightBufferData.lightPosition[mLightNumber] = math::XMFLOAT4(0.0f, sin(math::XMConvertToRadians(-153.0f)), cos(math::XMConvertToRadians(-153.0f)), 0.0f);
		}
		else
		{
			sLightBufferData.lightPosition[mLightNumber] = math::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	void Light::Update()
	{
		sLightBuffer->Update(&sLightBufferData);
	}
}
