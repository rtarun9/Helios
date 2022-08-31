#pragma once

#include "ComputeContext.hpp"
#include "PipelineState.hpp"
#include "Resources.hpp"
namespace helios::gfx
{
	// The device abstraction will have an object of this type.
	// Handles generation of mip maps for a given texture.
	class MipMapGenerator
	{
	public:
		MipMapGenerator(gfx::Device* device);

		void GenerateMips(gfx::Texture* texture);

		// Use this overload if UAV, SRV's and the mip map buffer are already created.
		// Useful for cases where mips are created for the same texture over and over again (such as in the Bloom render pass).
		void GenerateMips(gfx::Texture* texture, std::uint32_t srvIndex, std::span<uint32_t> uavIndices);

	private:
		std::unique_ptr<gfx::PipelineState> mMipMapPipelineState{};
		std::unique_ptr<gfx::Buffer> mMipMapBuffer{};

		gfx::Device& mDevice;
	};

}

