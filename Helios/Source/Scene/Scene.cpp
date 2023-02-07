#include "Scene/Scene.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "Core/FileSystem.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"

namespace helios::scene

{
    Scene::Scene(const gfx::GraphicsDevice* const graphicsDevice)
    {
        // Create scene buffer.
        m_sceneBuffer = graphicsDevice->createBuffer<interlop::SceneBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"Scene Buffer",
        });
        
        m_lights = std::make_unique<Lights>(graphicsDevice);
    }

    Scene::~Scene()
    {
    }

    void Scene::addModel(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc)
    {
        const std::wstring modelName{modelCreationDesc.modelName};

        m_modelFutures[modelName] = std::async(
            std::launch::async,
            [&](const gfx::GraphicsDevice* const graphicsDevice, const scene::ModelCreationDesc& modelCreationDesc) {
                return std::make_unique<scene::Model>(graphicsDevice, modelCreationDesc);
            },
            graphicsDevice, modelCreationDesc);
    }

    void Scene::addLight(const gfx::GraphicsDevice* device, const LightCreationDesc& lightCreationDesc)
    {
        const uint32_t lightCount = m_lights->m_currentLightCount;

        m_lights->m_lightsBufferData.lightColor[lightCount] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_lights->m_lightsBufferData.radiusIntensity[lightCount].x = 0.1f;
        m_lights->m_lightsBufferData.radiusIntensity[lightCount].y = 1.0f;

        if (lightCreationDesc.lightType == LightTypes::DirectionalLightData)
        {
            m_lights->m_lightsBufferData.lightPosition[lightCount] =
                math::XMFLOAT4(0.0f, sin(math::XMConvertToRadians(Lights::DIRECTIONAL_LIGHT_ANGLE)),
                               cos(math::XMConvertToRadians(Lights::DIRECTIONAL_LIGHT_ANGLE)), 0.0f);
        }
        else
        {
            m_lights->m_lightsBufferData.lightPosition[lightCount] = math::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        m_lights->m_currentLightCount++;
    }

    void Scene::completeResourceLoading()
    {
        for (auto& [name, modelFuture] : m_modelFutures)
        {
            m_models[name] = std::move(modelFuture.get());
        }

        m_modelFutures.clear();
    }

    void Scene::update(const float deltaTime, const core::Input& input, const float aspectRatio)
    {
        m_camera.update(deltaTime, input);

        const interlop::SceneBuffer sceneBufferData = {
            .viewProjectionMatrix =
                m_camera.computeAndGetViewMatrix() *
                math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(m_fov), aspectRatio, m_nearPlane, m_farPlane),
        };

        m_sceneBuffer.update(&sceneBufferData);

        for (auto& [name, model] : m_models)
        {
            model->getTransformComponent().update();
        }

        m_lights->update();
    }

    void Scene::renderModels(const gfx::GraphicsContext* const graphicsContext)
    {
        interlop::ModelViewerRenderResources modelViewerRenderResources = {
            .sceneBufferIndex = m_sceneBuffer.cbvIndex,
        };

        for (const auto& [name, model]: m_models)
        {
            model->render(graphicsContext, modelViewerRenderResources);
        }
    }

    void Scene::renderLights(const gfx::GraphicsContext* const graphicsContext)
    {
        interlop::LightRenderResources lightRenderResources = {
            .sceneBufferIndex = m_sceneBuffer.cbvIndex,
        };

        m_lights->render(graphicsContext, lightRenderResources);
    }
} // namespace helios::scene