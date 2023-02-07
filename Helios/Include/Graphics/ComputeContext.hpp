#pragma once

#include "Context.hpp"
#include "PipelineState.hpp"
#include "Resources.hpp"

namespace helios::gfx
{
    class DescriptorHeap;
    class GraphicsDevice;

    // Wrapper class for Compute CommandList, which provides a set of easy and simple functions to record commands for
    // execution by GPU. The Compute command list is of type Compute, which means it can issue compute commands.
    class ComputeContext : public Context
    {
      public:
        // note(rtarun9) : Not defining any of the big 5 member functions (destructor, move assignment etc) since the
        // compiler generated onces should suffice (no deep copies required here).
        explicit ComputeContext(GraphicsDevice* const device);

        void reset() override;

        void dispatch(const uint32_t threadGroupX, const uint32_t threadGroupY, const uint32_t threadGroupZ) const;
        void setDescriptorHeaps(const std::span<const DescriptorHeap* const> shaderVisibleDescriptorHeaps) const;

        // Configure pipeline / root signature related functions.
        void setComputeRootSignatureAndPipeline(const PipelineState& pipelineState) const;
        void set32BitComputeConstants(const void* renderResources) const;

      private:
        static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
        GraphicsDevice& graphicsDevice;
    };
} // namespace helios::gfx
