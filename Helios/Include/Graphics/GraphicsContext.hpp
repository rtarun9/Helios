#pragma once

#include "Context.hpp"
#include "PipelineState.hpp"
#include "Resources.hpp"

namespace helios::gfx
{
    class DescriptorHeap;
    class GraphicsDevice;

    // Wrapper class for Graphics CommandList, which provides a set of easy and simple functions to record commands for
    // execution by GPU. The GraphicsContext command list is of type Direct, which means it can issue basically all
    // commands (copy, compute, and rendering related).
    class GraphicsContext : public Context
    {
      public:
        // note(rtarun9) : Not defining any of the big 5 member functions (destructor, move assignment etc) since the
        // compiler generated onces should suffice (no deep copies required here).
        explicit GraphicsContext(GraphicsDevice* const device);

        void reset() override;

        void clearRenderTargetView(const Texture& renderTarget, const std::span<const float, 4> color);
        void clearRenderTargetView(const std::span<const Texture> renderTargets, const std::span<const float, 4> color);
        void clearRenderTargetView(const Texture& texture, const float color);
        void clearDepthStencilView(const Texture& texture);
        void setDescriptorHeaps() const;

        // Configure pipeline / root signature related functions.
        void setGraphicsPipelineState(const PipelineState& pipelineState) const;
        void setGraphicsRootSignature() const;
        void setGraphicsRootSignatureAndPipeline(const PipelineState& pipelineState) const;

        void setIndexBuffer(const Buffer& buffer) const;
        void set32BitGraphicsConstants(const void* renderResources) const;

        void setComputePipelineState(const PipelineState& pipelineState) const;
        void setComputeRootSignature() const;
        void setComputeRootSignatureAndPipeline(const PipelineState& pipelineState) const;
        void set32BitComputeConstants(const void* renderResources) const;

        void setViewport(const D3D12_VIEWPORT& viewport) const;

        void setPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY primitiveTopology) const;

        void setRenderTarget(const Texture& renderTarget) const;
        void setRenderTarget(const Texture& renderTarget, const Texture& depthStencilTexture) const;
        void setRenderTarget(const std::span<const Texture> renderTargets, const Texture& depthStencilTexture) const;

        void copyResource(ID3D12Resource* const source, ID3D12Resource* const destination) const;

        // Draw functions.
        void drawInstanceIndexed(const uint32_t indicesCount, const uint32_t instanceCount = 1u) const;
        void drawIndexed(const uint32_t indicesCount, const uint32_t instanceCount = 1u) const;

        // Dispatch functions.
        void dispatch(const uint32_t threadGroupDimX, const uint32_t threadGroupDimY, const uint32_t threadGroupDimZ);

      private:
        static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
        GraphicsDevice& graphicsDevice;
    };
} // namespace helios::gfx
