#include "Pch.hpp"

#include "Light.hpp"

using namespace DirectX;

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
			sLightBufferData.radius[mLightNumber] = 0.02f;
		}
	}

	void Light::Update()
	{
		// Make the model matrix.
		if (mLightType == LightTypes::PointLightData)
		{
			math::XMVECTOR translationVector = math::XMLoadFloat4(&sLightBufferData.lightPosition[mLightNumber]);

			static float angle = 0.0f;

			math::XMFLOAT4 inverseLightPosition = { sLightBufferData.lightPosition[mLightNumber].x * -1, 0.0f, sLightBufferData.lightPosition[mLightNumber].z * -1, 1.0f };
			math::XMVECTOR inverseTranslationVector = math::XMLoadFloat4(&inverseLightPosition);
			
			//math::XMFLOAT4 rotationLightPosition = {sin(angle) * 10, 0.0f, cos(angle) * 10, 1.0f};
			//math::XMVECTOR rotationTranslationVector = math::XMLoadFloat4(&rotationLightPosition);

			//sLightBufferData.lightPosition[mLightNumber] = rotationLightPosition;
			sLightInstanceData.modelMatrix[mLightNumber] = math::XMMatrixTranslationFromVector(inverseTranslationVector) * math::XMMatrixScaling(sLightBufferData.radius[mLightNumber], sLightBufferData.radius[mLightNumber], sLightBufferData.radius[mLightNumber]) *  math::XMMatrixTranslationFromVector(translationVector);
			//sLightInstanceData.modelMatrix[mLightNumber] =  math::XMMatrixScaling(sLightBufferData.radius[mLightNumber], sLightBufferData.radius[mLightNumber], sLightBufferData.radius[mLightNumber]) *  math::XMMatrixTranslationFromVector(rotationTranslationVector);

			sLightInstanceBuffer->Update(&sLightInstanceData);

			//angle += 0.01f;
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
