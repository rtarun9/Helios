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
}
