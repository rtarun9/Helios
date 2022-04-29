#include "Pch.hpp"

#include "Engine.hpp"

namespace helios
{
	Engine::Engine(Config& config)
		: m_Width{ config.width }, m_Height{ config.height }, m_Title{ config.title }
	{
		m_AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
	}

	// These two functions are empty for now, might be filled in the future.
	void Engine::OnKeyAction(uint8_t keycode, bool isKeyDown)
	{
	}

	void Engine::OnResize()
	{
	}
}