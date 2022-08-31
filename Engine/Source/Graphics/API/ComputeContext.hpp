#pragma once

#include "Context.hpp"

namespace helios::gfx
{
	class Device;

	// Wrapper class for Graphics CommandList, which provides a set of easy and simple functions to record commands for execution by GPU related to the ComputePipeline.
	// The command queue will contain a queue of command list, which can be passed into the ComputeContext 's constructor to create a ComputeContext object.
	// note (rtarun9) : This design is subject to change.
	class ComputeContext : public Context
	{
	public:
		ComputeContext(Device* device, const gfx::PipelineState* pipelineState);

		// Core functionalities.
		void Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) const;

		void SetDescriptorHeaps(const Descriptor* descriptor) const;

		void SetComputeRootSignature(PipelineState* pipelineState) const;
		void Set32BitComputeConstants(const void* renderResources) const;

		void SetComputePipelineState(PipelineState* pipelineState) const;

	private:
		static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;

		Device& mDevice;
	};
}


