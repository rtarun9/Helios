// clang-format off
#pragma once

#ifdef __cplusplus

#define uint uint32_t

#endif

namespace interlop
{
    struct TriangleRenderResources
    {
        uint sceneBufferIndex;
        uint positionBufferIndex;
        uint textureCoordBufferIndex;
        uint textureIndex;
    };

    struct ModelViewerRenderResources
    {
        uint sceneBufferIndex;
        uint transformBufferIndex;
        uint positionBufferIndex;
        uint normalBufferIndex;
        uint textureCoordBufferIndex;
        uint albedoTextureIndex;    
        uint albedoTextureSamplerIndex;
    };

    struct MipMapGenerationRenderResources
    {
        uint sourceMipIndex;

        uint outputMip1Index;
        uint outputMip2Index;
        uint outputMip3Index;
        uint outputMip4Index;

        uint mipMapGenerationBufferIndex;
    };

    struct LightRenderResources
{
    uint positionBufferIndex;

    uint lightBufferIndex;
    uint transformBufferIndex;

    uint sceneBufferIndex;
};
}
