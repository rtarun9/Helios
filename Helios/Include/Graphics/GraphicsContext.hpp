#pragma once

#include "Context.hpp"
#include "Resources.hpp"

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

        void clearRenderTargetView(BackBuffer& backBuffer, const std::span<const float, 4> color);
        void setDescriptorHeaps(const std::span<const DescriptorHeap* const> shaderVisibleDescriptorHeaps) const;

      private:
        static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
        GraphicsDevice& graphicsDevice;
    };
} // namespace helios::gfx
