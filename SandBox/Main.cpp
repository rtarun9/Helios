#include "Core/Application.hpp"

class SandBox final : public helios::core::Application
{
  public:
    SandBox(const std::string_view windowTitle) : Application(windowTitle)
    {
    }

    void loadContent() override{};
    void update(const float deltaTime) override{};
    void render() override{};

  private:
};

int main()
{
    SandBox sandbox{"Helios::SandBox"};
    sandbox.run();

    return 0;
}