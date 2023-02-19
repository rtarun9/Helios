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

        m_lights = Lights(graphicsDevice);
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
        if (m_lights->m_currentLightCount > interlop::MAX_LIGHTS)
        {
            return;
        }

        // Directional light is at index 0 always.
        if (lightCreationDesc.lightType == LightTypes::DirectionalLightData)
        {
            m_lights->m_lightsBufferData.lightColor[0] = {1.0f, 1.0f, 1.0f, 1.0f};
            m_lights->m_lightsBufferData.radiusIntensity[0].x = 0.1f;
            m_lights->m_lightsBufferData.radiusIntensity[0].y = 8.61f;

            m_lights->m_lightsBufferData.lightPosition[0] =
                math::XMFLOAT4(0.0f, sin(math::XMConvertToRadians(Lights::DIRECTIONAL_LIGHT_ANGLE)),
                               cos(math::XMConvertToRadians(Lights::DIRECTIONAL_LIGHT_ANGLE)), 0.0f);
        }
        else
        {
            m_lights->m_lightsBufferData.lightColor[m_lights->m_currentLightCount] =
                math::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

            m_lights->m_lightsBufferData.lightPosition[m_lights->m_currentLightCount] = math::XMFLOAT4(lightCreationDesc.worldSpaceLightPosition.x, lightCreationDesc.worldSpaceLightPosition.y,
                               lightCreationDesc.worldSpaceLightPosition.z, 1.0f);
            m_lights->m_lightsBufferData.radiusIntensity[m_lights->m_currentLightCount].x = 0.1f;
            m_lights->m_lightsBufferData.radiusIntensity[m_lights->m_currentLightCount].y = 1.0f;

            m_lights->m_currentLightCount++;
        }
    }

    void Scene::addCubeMap(gfx::GraphicsDevice* const graphicsDevice, const CubeMapCreationDesc& cubeMapCreationDesc)
    {
        m_cubeMap = CubeMap(graphicsDevice, cubeMapCreationDesc);
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
            .viewMatrix = m_camera.computeAndGetViewMatrix(),
            .projectionMatrix =
                math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(m_fov), aspectRatio, m_nearPlane, m_farPlane),
            .inverseViewMatrix = math::XMMatrixInverse(nullptr, m_camera.computeAndGetViewMatrix()),
            .viewProjectionMatrix =
                m_camera.computeAndGetViewMatrix() *
                math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(m_fov), aspectRatio, m_nearPlane, m_farPlane),
        };

        m_sceneBuffer.update(&sceneBufferData);

        for (auto& [name, model] : m_models)
        {
            model->getTransformComponent().update();
            model->updateMaterialBuffer();
        }

        m_lights->update(sceneBufferData.viewMatrix);
    }

    void Scene::renderModels(const gfx::GraphicsContext* const graphicsContext)
    {
        interlop::ModelViewerRenderResources modelViewerRenderResources = {
            .sceneBufferIndex = m_sceneBuffer.cbvIndex,
        };

        for (const auto& [name, model] : m_models)
        {
            model->render(graphicsContext, modelViewerRenderResources);
        }
    }

    void Scene::renderModels(const gfx::GraphicsContext* const graphicsContext,
                             const interlop::BlinnPhongRenderResources& renderResources)
    {
        interlop::BlinnPhongRenderResources blinnPhongRenderResources = renderResources;
        blinnPhongRenderResources.sceneBufferIndex = m_sceneBuffer.cbvIndex;
        blinnPhongRenderResources.lightBufferIndex = m_lights->m_lightsBuffer.cbvIndex;

        graphicsContext->set32BitGraphicsConstants(&blinnPhongRenderResources);
        graphicsContext->drawInstanceIndexed(3u);
    }

    void Scene::renderModels(const gfx::GraphicsContext* const graphicsContext,
                             const interlop::PBRRenderResources& renderResources)
    {
        interlop::PBRRenderResources pbrRenderResources = renderResources;
        pbrRenderResources.sceneBufferIndex = m_sceneBuffer.cbvIndex;
        pbrRenderResources.lightBufferIndex = m_lights->m_lightsBuffer.cbvIndex;

        graphicsContext->set32BitGraphicsConstants(&pbrRenderResources);
        graphicsContext->drawInstanceIndexed(3u);
    }

    void Scene::renderModels(const gfx::GraphicsContext* const graphicsContext,
                             const interlop::DeferredGPassRenderResources& renderResources)
    {
        interlop::DeferredGPassRenderResources deferredGPassenderResources = {
            .sceneBufferIndex = m_sceneBuffer.cbvIndex,
        };

        for (const auto& [name, model] : m_models)
        {
            model->render(graphicsContext, deferredGPassenderResources);
        }
    }

    void Scene::renderModels(const gfx::GraphicsContext* const graphicsContext,
                             const interlop::ShadowPassRenderResources& renderResources)
    {
        interlop::ShadowPassRenderResources shadowPassRenderResources = renderResources;

        for (const auto& [name, model] : m_models)
        {
            model->render(graphicsContext, shadowPassRenderResources);
        }
    }
    void Scene::renderLights(const gfx::GraphicsContext* const graphicsContext)
    {
        interlop::LightRenderResources lightRenderResources = {
            .sceneBufferIndex = m_sceneBuffer.cbvIndex,
        };

        m_lights->render(graphicsContext, lightRenderResources);
    }

    void Scene::renderCubeMap(const gfx::GraphicsContext* const graphicsContext, const uint32_t cubeMapTextureIndex)
    {
        interlop::CubeMapRenderResources cubeMapRenderResources = {
            .sceneBufferIndex = m_sceneBuffer.cbvIndex,
            .textureIndex = cubeMapTextureIndex,
        };

        m_cubeMap->render(graphicsContext, cubeMapRenderResources);
    }
} // namespace helios::scene