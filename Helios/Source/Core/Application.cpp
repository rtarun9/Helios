#include "Core/Application.hpp"

#include <SDL.h>
#include <SDL_syswm.h>

namespace helios::core
{
    Application::Application(const std::string_view windowTitle) : m_windowTitle(windowTitle)
    {
    }

    Application::~Application()
    {
        cleanup();
    }

    void Application::initPlatformBackend()
    {
        // Set DPI awareness.
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

        // Initialize SDL2 and create window.
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            fatalError("Failed to initialize SDL2.");
        }

        // Get monitor dimensions.
        SDL_DisplayMode displayMode{};
        if (SDL_GetCurrentDisplayMode(0, &displayMode) < 0)
        {
            fatalError(std::format("Failed to get display mode. SDL Error: {}", SDL_GetError()));
        }

        const uint32_t monitorWidth = displayMode.w;
        const uint32_t monitorHeight = displayMode.h;

        // Window must cover 60% of the screen.
        m_windowWidth = static_cast<uint32_t>(monitorWidth * 0.60f);
        m_windowHeight = static_cast<uint32_t>(monitorHeight * 0.60f);

        m_window = SDL_CreateWindow(m_windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    m_windowWidth, m_windowHeight, SDL_WINDOW_ALLOW_HIGHDPI);

        if (!m_window)
        {
            fatalError(std::format("Failed to create SDL2 window. SDL Error: {}", SDL_GetError()));
        }

        // Get the underlying window handle.
        SDL_SysWMinfo wmInfo{};
        SDL_VERSION(&wmInfo.version);

        SDL_GetWindowWMInfo(m_window, &wmInfo);
        m_windowHandle = wmInfo.info.win.window;
    }

    void Application::init()
    {
        initPlatformBackend();

        // Initialize the graphics device.
        m_graphicsDevice = std::make_unique<gfx::GraphicsDevice>(m_windowWidth, m_windowHeight,
                                                                 DXGI_FORMAT_R10G10B10A2_UNORM, m_windowHandle);
    }

    void Application::cleanup()
    {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void Application::run()
    {
        try
        {
            init();
            loadContent();

            std::chrono::high_resolution_clock clock{};
            std::chrono::high_resolution_clock::time_point previousFrameTimePoint{};

            bool quit = false;
            while (!quit)
            {
                SDL_Event event{};
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_QUIT)
                    {
                        quit = true;
                    }

                    const uint8_t* keyboardState = SDL_GetKeyboardState(nullptr);
                    if (keyboardState[SDL_SCANCODE_ESCAPE])
                    {
                        quit = true;
                    }
                }

                const std::chrono::high_resolution_clock::time_point currentFrameTimePoint = clock.now();
                const float deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentFrameTimePoint -
                                                                                              previousFrameTimePoint)
                                            .count();
                previousFrameTimePoint = currentFrameTimePoint;

                update(deltaTime);
                render();
            }
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[EXCEPTION] :: " << exception.what() << '\n';
            return;
        }
    }
} // namespace helios::core