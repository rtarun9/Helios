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

		gfx::BufferCreationDesc lightInstanceBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Light Instance Constant Buffer"
		};

		sLightInstanceBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<InstanceLightBuffer>(lightInstanceBufferCreationDesc, std::span<InstanceLightBuffer, 0u>{}));
		sLightInstanceData = {};

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
		sLightInstanceBuffer->allocation->Reset();
	}

	Light::Light(const gfx::Device* device, const LightCreationDesc& lightCreationDesc) : mLightNumber(lightCreationDesc.lightNumber), mLightType(lightCreationDesc.lightType)
	{
		// The value 0.001 is hardcoded, which is not ideal.
		// The sphere model is very large, which is why this happens.
		sLightBufferData.lightColor[mLightNumber] = { 1.0f, 1.0f, 1.0f, 1.0f };
		sLightBufferData.radius[mLightNumber] = 0.001f;

		if (lightCreationDesc.lightType == LightTypes::DirectionalLightData)
		{
			sLightBufferData.lightPosition[mLightNumber] = math::XMFLOAT4(0.0f, sin(math::XMConvertToRadians(DIRECTIONAL_LIGHT_ANGLE)), cos(math::XMConvertToRadians(DIRECTIONAL_LIGHT_ANGLE)), 0.0f);
		}
		else
		{
			sLightBufferData.lightPosition[mLightNumber] = math::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	void Light::Update()
	{
		// Make the model matrix.
		if (mLightType == LightTypes::PointLightData)
		{
			math::XMVECTOR translationVector = math::XMLoadFloat4(&sLightBufferData.lightPosition[mLightNumber]);
			sLightInstanceData.modelMatrix[mLightNumber] = math::XMMatrixScaling(0.02f, 0.02f, 0.02f) *  math::XMMatrixTranslationFromVector(translationVector);

			sLightInstanceBuffer->Update(&sLightInstanceData);
		}
	}

	void Light::UpdateLightBuffer()
	{	
		sLightBuffer->Update(&sLightBufferData);
	}

	void Light::Render(const gfx::GraphicsContext* graphicsContext, LightRenderResources& lightRenderResources)
	{
		lightRenderResources.lightBufferIndex = sLightBuffer->cbvIndex;
		lightRenderResources.transformBufferIndex = sLightInstanceBuffer->cbvIndex;

		sLightModel->Render(graphicsContext, lightRenderResources);
	}
}
