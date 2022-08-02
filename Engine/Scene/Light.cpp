#include "Pch.hpp"

#include "Light.hpp"

namespace helios::scene
{
	void Light::CreateLightResources(const gfx::Device* device)
	{
		gfx::BufferCreationDesc lightDataBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Light Constant Buffer",
		};

		sLightBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<LightBuffer>(lightDataBufferCreationDesc, std::span<const LightBuffer, 0u>{}));
		sLightBufferData = {};

		ModelCreationDesc lightModelCreationDesc
		{
			.modelPath = LIGHT_MODEL_PATH,
			.modelName = L"Light Model",
		};

		sLightModel = std::make_unique<Model>(device, lightModelCreationDesc);
	}

	void Light::DestroyLightResources()
	{
		sLightModel.reset();
		sLightBuffer->allocation->Reset();
	}

	Light::Light(const gfx::Device* device, const LightCreationDesc& lightCreationDesc) : mLightNumber(lightCreationDesc.lightNumber), mLightType(lightCreationDesc.lightType)
	{
		gfx::BufferCreationDesc transformBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name =  (lightCreationDesc.lightType == LightTypes::DirectionalLightData ? L" Directional Light Transform Buffer "  : L"Point Light Transform Buffer") + std::to_wstring(lightCreationDesc.lightNumber),
		};

		mTransform.transformBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<TransformBuffer>(transformBufferCreationDesc, std::span<TransformBuffer, 0u>{}));

		// The value 0.001 is hardcoded, which is not ideal.
		// The sphere model is very large, which is why this happens.
		sLightBufferData.lightColor[mLightNumber] = { 1.0f, 1.0f, 1.0f, 1.0f };
		sLightBufferData.radius[mLightNumber] = 0.001f;

		if (lightCreationDesc.lightType == LightTypes::DirectionalLightData)
		{
			mTransform.data.translate = math::XMFLOAT3(0.0f, sin(math::XMConvertToRadians(DIRECTIONAL_LIGHT_ANGLE)), cos(math::XMConvertToRadians(DIRECTIONAL_LIGHT_ANGLE)));
		}
		else
		{
			mTransform.data.translate = math::XMFLOAT3(1.0f, 1.0f, 1.0f);
		}
	}

	void Light::Update()
	{
		sLightBufferData.lightPosition[mLightNumber] = 
		{ 
			mTransform.data.translate.x, 
			mTransform.data.translate.y, 
			mTransform.data.translate.z,
			mLightType == LightTypes::DirectionalLightData ? 0.0f : 1.0f
		};

		mTransform.Update();
	}

	void Light::UpdateLightBuffer()
	{	
		sLightBuffer->Update(&sLightBufferData);
	}
}
