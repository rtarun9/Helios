#include "Pch.hpp"

#include "Engine.hpp"

namespace helios
{
	Engine::Engine(Config& config)
		: mWidth{ config.width }, mHeight{ config.height }, mTitle{ config.title }
	{
		mAspectRatio = static_cast<float>(mWidth) / static_cast<float>(mHeight);
	}

	// These two functions are empty for now, might be filled in the future.
	void Engine::OnKeyAction(uint8_t keycode, bool isKeyDown)
	{
	}

	void Engine::OnResize()
	{
	}
}