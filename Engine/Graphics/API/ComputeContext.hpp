#pragma once

#include "Pch.hpp"

#include "PipelineState.hpp"
#include "Resources.hpp"
#include "Descriptor.hpp"

namespace helios::gfx
{
	class Device;

	// Wrapper class for Graphics CommandList, which provides a set of easy and simple functions to record commands for execution by GPU related to the ComputePipeline.
	// The command queue will contain a queue of command list, which can be passed into the ComputeContext 's constructor to create a ComputeContext object.
	// note (rtarun9) : This design is subject to change.
	class ComputeContext
	{
	public:
		ComputeContext(Device& device);
		ID3D12GraphicsCommandList* const GetCommandList() const { return mCommandList.Get(); }

		// Core functionalities.
		void Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) const;

		void SetDescriptorHeaps(const Descriptor* descriptor) const;

	private:
		static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList{};

		Device& mDevice;
	};
}


