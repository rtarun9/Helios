#pragma once

#include "Core/Input.hpp"

#include "Graphics/GraphicsDevice.hpp"
#include "Scene/Scene.hpp"
#include "Editor/Editor.hpp"


struct SDL_Window;

namespace helios::core
{
    // All Helios 'Applications', are required to inherit from this class.
    // The main game loop is located within the application class.
    // The application handles creation of the GraphicsDevice, Editor and Scene abstractions.
    class Application
    {
      public:
        explicit Application(const std::string_view windowTitle);
        virtual ~Application();

        Application(const Application& other) = delete;
        Application& operator=(const Application& other) = delete;

        Application(Application&& other) = delete;
        Application& operator=(Application&& other) = delete;

        void run();

      private:
        void initPlatformBackend();
        void initScene();
        void initEditor();

      protected:
        virtual void init();
        virtual void cleanup();

        virtual void loadContent() = 0;
        virtual void update(const float deltaTime) = 0;
        virtual void render() = 0;

      protected:
        std::string m_windowTitle{};

        uint32_t m_windowWidth{};
        uint32_t m_windowHeight{};
        SDL_Window* m_window{};
        HWND m_windowHandle{};

        std::unique_ptr<gfx::GraphicsDevice> m_graphicsDevice{};

        Input m_input{};
        std::unique_ptr<scene::Scene> m_scene{};
        std::unique_ptr<editor::Editor> m_editor{};
    };
} // namespace helios::core