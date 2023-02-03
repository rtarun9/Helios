#pragma once

#include "Graphics/GraphicsDevice.hpp"

struct SDL_Window;

namespace helios::core
{
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
    };
} // namespace helios::core