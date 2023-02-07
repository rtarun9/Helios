#pragma once

#include "../Graphics/Resources.hpp"

namespace helios::scene
{
    // Stores all mesh data.
    struct Mesh
    {
        gfx::Buffer positionBuffer{};
        gfx::Buffer textureCoordsBuffer{};
        gfx::Buffer normalBuffer{};

        gfx::Buffer indexBuffer{};

        uint32_t indicesCount{};

        uint32_t materialIndex{};
    };
} // namespace helios::scene