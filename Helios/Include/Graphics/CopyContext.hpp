#pragma once

#include "Context.hpp"
#include "Resources.hpp"

namespace helios::gfx
{
    class GraphicsDevice;

    // Wrapper class for Copy CommandList, which provides a set of easy and simple functions related to copying data
    // from one resource to another.
    class CopyContext : public Context
    {
      public:
        // note(rtarun9) : Not defining any of the big 5 member functions (destructor, move assignment etc) since the
        // compiler generated onces should suffice (no deep copies required here).
        explicit CopyContext(GraphicsDevice* const device);

      private:
        static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
        GraphicsDevice& graphicsDevice;
    };
} // namespace helios::gfx
