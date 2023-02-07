#pragma once

#include "Core/Input.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Model.hpp"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{
    // The reason for this abstraction is to separate the code for managing scene objects (camera / model / light) from
    // the SandBox, which is mostly related to rendering techniques and other stuff. However, the scene will not hold a
    // reference to the gfx::Device as this class is mostly handled from engine. Note that all member variables are
    // public, can be freely accessed from anywhere.
    class Scene
    {
      public:
        explicit Scene(const gfx::GraphicsDevice* const graphicsDevice);
        ~Scene();

      public:
        void addModel(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc);

        // NOTE : Not to be used by user, the application will automatically call then function when required.
        // Make sure all background threads that were used to load resources are completed.
        // If not, block the main thread and wait for resources to finish loading.
        void completeResourceLoading();

        void update(const float deltaTime, const core::Input& input, const float aspectRatio);

        // Render models using the Model View render resources.
        void renderModels(const gfx::GraphicsContext* const graphicsContext);

      public:
        gfx::Buffer m_sceneBuffer{};
        Camera m_camera{};

        float m_nearPlane{0.1f};
        float m_farPlane{150.0f};
        float m_fov{45.0f};

        std::unordered_map<std::wstring, std::future<std::unique_ptr<Model>>> m_modelFutures{};
        std::unordered_map<std::wstring, std::unique_ptr<Model>> m_models{};
    };

} // namespace helios::scene