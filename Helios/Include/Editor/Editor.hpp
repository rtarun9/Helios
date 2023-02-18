#pragma once

#include "../Graphics/Resources.hpp"
#include "../Rendering/BloomPass.hpp"
#include "../Rendering/DeferredGeometryPass.hpp"
#include "../Rendering/PCFShadowMappingPass.hpp"
#include "../Rendering/SSAOPass.hpp"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{
    class Scene;
}

struct SDL_Window;

namespace helios::editor
{
    // The editor handles all UI related task.
    // The goal is to avoid having UI specific configurations / settings in other abstractions, and have everything
    // contained within this class.
    class Editor
    {
      public:
        explicit Editor(const gfx::GraphicsDevice* const graphicsDevice, SDL_Window* const window, const uint32_t width,
                        const uint32_t height);
        ~Editor();

        // Goal is to call this single function from the engine which does all the UI internally, helps make the engine
        // clean as well. This function is heavy WIP and not given as much importance as other abstractions.
        void render(const gfx::GraphicsDevice* const graphicsDevice, scene::Scene* const scene,
                    rendering::DeferredGeometryBuffer& deferredGBuffer,
                    rendering::PCFShadowMappingPass& shadowMappingPass, rendering::SSAOPass& ssaoPass,
                    rendering::BloomPass& bloomPass, interlop::PostProcessingBuffer& postProcessBuffer,
                    gfx::Texture& renderTarget, gfx::GraphicsContext* const graphicsContext);

        void showUI(const bool value);

        void resize(const uint32_t width, const uint32_t height) const;

      private:
        void renderSceneHierarchy(scene::Scene* const scene) const;

        void renderMaterialProperties(const gfx::GraphicsDevice* const graphicsDevice, scene::Scene* const scene) const;

        void renderLightProperties(scene::Scene* const scene) const;

        // Handles camera and other scene related properties.
        void renderSceneProperties(scene::Scene* const scene) const;

        void renderDeferredGBuffer(const gfx::GraphicsDevice* const graphicsDevice,
                                   const rendering::DeferredGeometryBuffer& deferredGBuffer) const;

        void renderShadowMappingPass(const gfx::GraphicsDevice* const graphicsDevice,
                                     rendering::PCFShadowMappingPass& shadowMappingPass) const;

        void renderSSAOPass(const gfx::GraphicsDevice* const graphicsDevice, rendering::SSAOPass& ssaoPass) const;

        void renderBloomPass(const gfx::GraphicsDevice* const graphicsDevice, rendering::BloomPass& bloomPass) const;

        void renderPostProcessingProperties(interlop::PostProcessingBuffer& postProcessBufferData) const;

        // Accepts the pay load (accepts data which is dragged in from content browser to the scene view port, and loads
        // the model (if path belongs to a .gltf file).
        void renderSceneViewport(const gfx::GraphicsDevice* device, gfx::Texture& renderTarget,
                                 scene::Scene* const scene) const;

        // Handle drag and drop of models into view port at run time.
        void renderContentBrowser();

      private:
        bool m_showUI{true};

        std::filesystem::path m_contentBrowserCurrentPath{};

        // Path to .ini file.
        std::string m_iniFilePath{};

        // Passed in from the application.
        SDL_Window* m_window{};
    };
} // namespace helios::editor