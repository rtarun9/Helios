#pragma once

#include "Pch.hpp"

#include "ComputeContext.hpp"
#include "PipelineState.hpp"

namespace helios::gfx
{
	// The device abstraction will have an object of this type.
	// Handles generation of mip maps for a given texture.
	// note(rtarun9) : Entire class needs to be rethought and modified to check if dimensions are odd / even etc.
	// (something like this : https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli).
	class MipMapGenerator
	{
	public:
		MipMapGenerator(gfx::Device* device);

		void GenerateMips(gfx::Texture* texture);

	public:
		static constexpr uint32_t MAX_MIP_LEVELS = 6u;

	private:
		std::unique_ptr<gfx::PipelineState> mMipMapPipelineState{};

		gfx::Device& mDevice;
	};

}

