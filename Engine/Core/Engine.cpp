#include "Pch.hpp"

#include "Engine.hpp"

namespace helios::core
{
	Engine::Engine(Config& config)
		:  mTitle{ config.title }, mDimensions{config.dimensions}
	{
		mAspectRatio = static_cast<float>(config.dimensions.x) / static_cast<float>(config.dimensions.y);
	}
}