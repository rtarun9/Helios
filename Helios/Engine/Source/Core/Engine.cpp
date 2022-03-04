#include "Pch.hpp"

#include "Include/Core/Engine.hpp"

namespace helios
{
	Engine::Engine(Config& config)
		: m_Width{ config.width }, m_Height{ config.height }, m_Title{ config.title }
	{
		m_AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
	}

	void Engine::OnKeyAction(uint8_t keycode, bool isKeyDown)
	{
	}

	void Engine::OnResize()
	{
	}

	uint32_t Engine::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Engine::GetHeight() const
	{
		return m_Height;
	}

	std::wstring Engine::GetTitle() const
	{
		return m_Title;
	}
}