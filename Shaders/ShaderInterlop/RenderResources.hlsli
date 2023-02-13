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

    struct BlinnPhongRenderResources
    {
        uint sceneBufferIndex;
        uint lightBufferIndex;
        uint albedoGBufferIndex;
        uint positionEmissiveGBufferIndex;
        uint normalEmissiveGBufferIndex;
        uint aoMetalRoughnessEmissiveGBufferIndex;
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

    struct PostProcessingRenderResources
    {
        uint renderTextureIndex;
    };

    struct FullScreenTrianglePassRenderResources
    {
        uint renderTextureIndex;
    };

    struct DeferredGPassRenderResources
    {
        uint positionBufferIndex;
        uint textureCoordBufferIndex;
        uint normalBufferIndex;

        uint transformBufferIndex;

        uint sceneBufferIndex;

        uint albedoTextureIndex;
        uint albedoTextureSamplerIndex;

        uint metalRoughnessTextureIndex;
        uint metalRoughnessTextureSamplerIndex;

        uint normalTextureIndex;
        uint normalTextureSamplerIndex;

        uint aoTextureIndex;
        uint aoTextureSamplerIndex;

        uint emissiveTextureIndex;
        uint emissiveTextureSamplerIndex;

        uint materialBufferIndex;
    };

    struct PBRRenderResources
    {
        uint sceneBufferIndex;
        uint lightBufferIndex;
        uint albedoGBufferIndex;
        uint positionEmissiveGBufferIndex;
        uint normalEmissiveGBufferIndex;
        uint aoMetalRoughnessEmissiveGBufferIndex;

        uint irradianceTextureIndex;
        uint prefilterTextureIndex;
        uint brdfLUTTextureIndex;
    };

    struct CubeFromEquirectRenderResources
    {
        uint textureIndex;
        uint outputTextureIndex;
    };

    struct CubeMapRenderResources
    {
        uint positionBufferIndex;
        uint sceneBufferIndex;
        uint textureIndex;
    };

    struct IrradianceRenderResources
    {
        uint skyBoxTextureIndex;
        uint ouputIrradianceMapIndex;
    };

    struct PreFilterRenderResources
    {
        uint skyBoxTextureIndex;
        uint outputPreFilteredCubeMapIndex;
        uint mipLevel;
    };

    struct BRDFLutRenderResources
    {
        uint lutTextureIndex;   
    };
}
