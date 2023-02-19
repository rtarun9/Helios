#pragma once

#include "Core/Input.hpp"
#include "Scene/Camera.hpp"
#include "Scene/CubeMap.hpp"
#include "Scene/Lights.hpp"
#include "Scene/Model.hpp"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{
    // The reason for this abstraction is to separate the code for managing scene objects (camera / model / light / cube
    // map) from the SandBox, which is mostly related to rendering techniques and other stuff. Note that all member
    // variables are public, can be freely accessed from anywhere.
    class Scene
    {
      public:
        explicit Scene(const gfx::GraphicsDevice* const graphicsDevice);

      public:
        void addModel(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc);
        void addLight(const gfx::GraphicsDevice* const graphicsDevice, const LightCreationDesc& lightCreationDesc);
        void addCubeMap(gfx::GraphicsDevice* const graphicsDevice,
                        const CubeMapCreationDesc& cubeMapCreationDesc);

        // NOTE : The application will call this function, but the user can call it as well if their
        // use case requires it.
        void completeResourceLoading();

        // Update scene resources (models, lights, etc).
        void update(const float deltaTime, const core::Input& input, const float aspectRatio);

        // Render models using various render resources.
        void renderModels(const gfx::GraphicsContext* const graphicsContext);
        void renderModels(const gfx::GraphicsContext* const graphicsContext,
                          const interlop::BlinnPhongRenderResources& renderResources);
        void renderModels(const gfx::GraphicsContext* const graphicsContext,
                          const interlop::PBRRenderResources& renderResources);
        void renderModels(const gfx::GraphicsContext* const graphicsContext,
                          const interlop::DeferredGPassRenderResources& renderResources);
        void renderModels(const gfx::GraphicsContext* const graphicsContext,
                          const interlop::ShadowPassRenderResources& renderResources);

        void renderLights(const gfx::GraphicsContext* const graphicsContext);

        void renderCubeMap(const gfx::GraphicsContext* const graphicsContext, const uint32_t cubeMapTextureIndex = INVALID_INDEX_U32);

      public:
        gfx::Buffer m_sceneBuffer{};
        Camera m_camera{};

        float m_nearPlane{0.1f};
        float m_farPlane{300.0f};
        float m_fov{45.0f};

        std::optional<Lights> m_lights{};
        std::optional<CubeMap> m_cubeMap{};

        std::unordered_map<std::wstring, std::unique_ptr<Model>> m_models{};

        std::unordered_map<std::wstring, std::future<std::unique_ptr<Model>>> m_modelFutures{};
    };

} // namespace helios::scene