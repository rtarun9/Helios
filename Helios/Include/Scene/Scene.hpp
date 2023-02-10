#pragma once

#include "Core/Input.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Model.hpp"
#include "Scene/Lights.hpp"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{
    // The reason for this abstraction is to separate the code for managing scene objects (camera / model / light) from
    // the SandBox, which is mostly related to rendering techniques and other stuff. Note that all member variables are
    // public, can be freely accessed from anywhere.
    class Scene
    {
      public:
        explicit Scene(const gfx::GraphicsDevice* const graphicsDevice);
        ~Scene() = default;

      public:
        void addModel(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc);
        void addLight(const gfx::GraphicsDevice* const graphicsDevice, const LightCreationDesc& lightCreationDesc);
		
        // NOTE : Not to be used by user, the application will automatically call then function when required.
        // Make sure all background threads that were used to load resources are completed.
        // If not, block the main thread and wait for resources to finish loading.
        void completeResourceLoading();

        // Update scene resources (models, lights, etc).
        void update(const float deltaTime, const core::Input& input, const float aspectRatio);

        // Render models using various render resources.
        void renderModels(const gfx::GraphicsContext* const graphicsContext);
        void renderLights(const gfx::GraphicsContext* const graphicsContext);
        void renderModels(const gfx::GraphicsContext* const graphicsContext, const interlop::BlinnPhongRenderResources& renderResources);
        void renderModels(const gfx::GraphicsContext* const graphicsContext, const interlop::PBRRenderResources& renderResources);
        void renderModels(const gfx::GraphicsContext* const graphicsContext, const interlop::DeferredGPassRenderResources& renderResources);

      public:
        gfx::Buffer m_sceneBuffer{};
        Camera m_camera{};

        float m_nearPlane{0.1f};
        float m_farPlane{300.0f};
        float m_fov{45.0f};
        
        std::unique_ptr<Lights> m_lights{};

        std::unordered_map<std::wstring, std::future<std::unique_ptr<Model>>> m_modelFutures{};
        std::unordered_map<std::wstring, std::unique_ptr<Model>> m_models{};
    };

} // namespace helios::scene