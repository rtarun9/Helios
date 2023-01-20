#pragma once

struct SDL_Window;

namespace helios::core
{
    class Application
    {
      public:
        Application(const std::string_view windowTitle);
        virtual ~Application();

        void run();

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
    };
}