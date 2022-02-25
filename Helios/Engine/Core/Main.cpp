#include "Pch.hpp"

#include "Application.hpp"
#include "SandBox.hpp"

int WINAPI wWinMain(HINSTANCE instance, [[maybe_unused]] HINSTANCE prevInstance, [[maybe_unused]] LPWSTR commandLine, [[maybe_unused]] INT commandShow)
{
	helios::Config config
	{
		.title = L"Helios Engine",
		.width = 192,
		.height = 108,
	};

	helios::SandBox sandBox{ config };

	return helios::Application::Run(&sandBox, instance);
}