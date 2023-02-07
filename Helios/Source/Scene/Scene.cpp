#include "Scene/Scene.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include "Core/ResourceManager.hpp"

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
} // namespace helios::scene