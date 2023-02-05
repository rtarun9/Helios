#pragma once

#include "../Graphics/Resources.hpp"

namespace helios::scene
{
    // This struct stores the texture's required for a PBR material. If a texture does not exist, the shader resource
    // view index will be set to INVALID_INDEX_U32 (which is set by default in the sturct Texture). The shader will
    // accordingly set a null view or not use that particular texture. Each texture (if it exist) will have a sampler
    // index associated with it, so we can use SamplerDescriptorHeap to index into the heap directly. The same applies
    // for samplers.
    struct PBRMaterial
    {
        gfx::Texture albedoTexture{};
        gfx::Sampler albedoTextureSampler{};

        gfx::Texture normalTexture{};
        gfx::Sampler normalTextureSampler{};

        gfx::Texture metalRoughnessTexture{};
        gfx::Sampler metalRoughnessTextureSampler{};

        gfx::Texture aoTexture{};
        gfx::Sampler aoTextureSampler{};

        gfx::Texture emissiveTexture{};
        gfx::Sampler emissiveTextureSampler{};
    };
} // namespace helios::scene