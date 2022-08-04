#include "Pch.hpp"

#include "Scene.hpp"
#include "Core/Application.hpp"

#include "Common/ConstantBuffers.hlsli"

namespace helios::scene
{
	Scene::Scene(const gfx::Device* device)
	{
		// Init light resources.
		Light::CreateLightResources(device);

		// Create scene buffer.
		// Load scene constant buffer.
		gfx::BufferCreationDesc sceneBufferCreationDesc
		{
			.usage = gfx::BufferUsage::ConstantBuffer,
			.name = L"Scene Buffer",
		};

		mSceneBuffer = std::make_unique<gfx::Buffer>(device->CreateBuffer<SceneBuffer>(sceneBufferCreationDesc, std::span<SceneBuffer, 0u>{}));
		
		mCamera = std::make_unique<Camera>();
	}
	
	Scene::~Scene()
	{
		Light::DestroyLightResources();
	}

	void Scene::AddModel(const gfx::Device* device, const ModelCreationDesc& modelCreationDesc)
	{
		auto model = std::make_unique<Model>(device, modelCreationDesc);
		mModels.push_back(std::move(model));
	}

	void Scene::AddModel(std::unique_ptr<Model> model)
	{
		mModels.push_back(std::move(model));
	}

	void Scene::AddLight(const gfx::Device* device, const LightCreationDesc& modelCreationDesc)
	{
		auto light = std::make_unique<Light>(device, modelCreationDesc);
		mLights.push_back(std::move(light));
	}

	void Scene::AddSkyBox(gfx::Device* device, const SkyBoxCreationDesc& skyBoxCreationDesc)
	{
		mSkyBox = std::make_unique<SkyBox>(device, skyBoxCreationDesc);
	}

	void Scene::AddCamera()
	{
	}

	void Scene::Update(float cameraAspectRatio)
	{
		mCamera->Update(static_cast<float>(core::Application::GetTimer().GetDeltaTime()));

		SceneBuffer sceneBufferData =
		{
			.cameraPosition = mCamera->GetCameraPosition(),
			.cameraTarget = mCamera->GetCameraTarget(),
			.viewMatrix = mCamera->GetViewMatrix(),
			.projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), cameraAspectRatio, 0.1f, 1000.0f),
		};

		mSceneBuffer->Update(&sceneBufferData);

		for (auto& model : mModels)
		{
			model->GetTransform()->Update();
		}

		for (auto& light : mLights)
		{
			light->Update();
		}

		scene::Light::UpdateLightBuffer();
	}

	void Scene::RenderModels(const gfx::GraphicsContext* graphicsContext)
	{
		SceneRenderResources sceneRenderResources
		{
			.sceneBufferIndex = mSceneBuffer->cbvIndex,
			.lightBufferIndex = scene::Light::GetCbvIndex()
		};

		for (auto& model : mModels)
		{
			model->Render(graphicsContext, sceneRenderResources);
		}
	}

	void Scene::RenderLights(const gfx::GraphicsContext* graphicsContext)
	{
		LightRenderResources lightRenderResources
		{
			.sceneBufferIndex = mSceneBuffer->cbvIndex
		};

		Light::Render(graphicsContext, lightRenderResources);
	}

	void Scene::RenderSkyBox(const gfx::GraphicsContext* graphicsContext)
	{
		SkyBoxRenderResources skyBoxRenderResources
		{
			.sceneBufferIndex = mSceneBuffer->cbvIndex,
		};

		mSkyBox->Render(graphicsContext, skyBoxRenderResources);
	}
}