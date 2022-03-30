#include "Pch.hpp"

#include "Application.hpp"
#include "SandBox.hpp"

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, [[maybe_unused]] _In_ LPWSTR commandLine, [[maybe_unused]] _In_ INT commandShow)
{
	helios::Config config
	{
		.title = L"Helios Engine",
		.width = 1920,
		.height = 1080,
	};

	SandBox sandBox{ config };

	return helios::Application::Run(&sandBox, instance);
}