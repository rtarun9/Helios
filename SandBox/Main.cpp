#include "Core/Application.hpp"

class SandBox final : public helios::core::Application
{
  public:
    explicit SandBox(const std::string_view windowTitle) : Application(windowTitle)
    {
    }

    void loadContent() override
    {
    }
    void update(const float deltaTime) override
    {
    }

    void render() override
    {
        m_graphicsDevice->beginFrame();

        auto& graphicsContext = m_graphicsDevice->getCurrentGraphicsContext();

        helios::gfx::BackBuffer& currentBackBuffer = m_graphicsDevice->getCurrentBackBuffer();

        // Prepare back buffer for rendering into it (i.e using it as a render target).
        graphicsContext->addResourceBarrier(currentBackBuffer.backBufferResource.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        graphicsContext->executeResourceBarriers();


        const std::array<float, 4> clearColor = {std::abs(std::cosf(m_frameCount/120.0f)), 0.0f, std::abs(std::sinf(m_frameCount/120.0f)), 1.0f};

        graphicsContext->clearRenderTargetView(currentBackBuffer, clearColor);

        // Prepare back buffer for presentation.
        graphicsContext->addResourceBarrier(currentBackBuffer.backBufferResource.Get(),
                                            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        graphicsContext->executeResourceBarriers();

        const std::array<helios::gfx::Context* const, 1u> contexts = {
            graphicsContext.get(),
        };

        m_graphicsDevice->getDirectCommandQueue()->executeContext(contexts);
    
        m_graphicsDevice->present();
        m_graphicsDevice->endFrame();

        m_frameCount++;
    }

  private:
    uint64_t m_frameCount{};
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}