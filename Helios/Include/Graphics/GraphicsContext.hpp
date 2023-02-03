#pragma once

#include "Context.hpp"
#include "Resources.hpp"
#include "PipelineState.hpp"

namespace helios::gfx
{
    class DescriptorHeap;
    class GraphicsDevice;

    // Wrapper class for Graphics CommandList, which provides a set of easy and simple functions to record commands for
    // execution by GPU.
    class GraphicsContext : public Context
    {
      public:
        // note(rtarun9) : Not defining any of the big 5 member functions (destructor, move assignment etc) since the
        // compiler generated onces should suffice (no deep copies required here).
        explicit GraphicsContext(GraphicsDevice* const device);

        void reset() override;

        void clearRenderTargetView(BackBuffer& backBuffer, const std::span<const float, 4> color);
        void setDescriptorHeaps(const std::span<const DescriptorHeap* const> shaderVisibleDescriptorHeaps) const;

        // COnfigure pipeline / root signature related functions.
        void setGraphicsPipelineState(const PipelineState& pipelineState) const;
        void setGraphicsRootSignature(const PipelineState& pipelineState) const;
        void setGraphicsRootSignatureAndPipeline(const PipelineState& pipelineState) const;

        void setIndexBuffer(const Buffer& buffer) const;
        void set32BitGraphicsConstants(const void* renderResources) const;

        void setViewport(const D3D12_VIEWPORT& viewport) const;

        void setPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY primitiveTopology) const;

        void setRenderTarget(const BackBuffer& renderTarget) const;

        // Draw functions.
        void drawInstanceIndexed(const uint32_t indicesCount, const uint32_t instanceCount = 1u) const;
        void drawIndexed(const uint32_t indicesCount, const uint32_t instanceCount = 1u) const;

      private:
        static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
        GraphicsDevice& graphicsDevice;
    };
} // namespace helios::gfx
