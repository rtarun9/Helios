// clang-format off
#pragma once

#ifdef __cplusplus

#define uint uint32_t
#define float2 math::XMFLOAT2

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
        uint albedoEmissiveGBufferIndex;
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
        uint postProcessBufferIndex;
        uint renderTextureIndex;
        uint ssaoTextureIndex;
        uint bloomTextureIndex;
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
        uint albedoEmissiveGBufferIndex;
        uint normalEmissiveGBufferIndex;
        uint aoMetalRoughnessEmissiveGBufferIndex;

        uint irradianceTextureIndex;
        uint prefilterTextureIndex;
        uint brdfLUTTextureIndex;

        uint shadowBufferIndex;
        uint shadowDepthTextureIndex;

        uint blurredSSAOTextureIndex;
        
        uint depthTextureIndex;
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
    
    struct ShadowPassRenderResources
    {
        uint positionBufferIndex;
        uint transformBufferIndex;
        uint shadowBufferIndex;
    };
   
    struct SSAORenderResources
    {
        uint normalTextureIndex;
        uint depthTextureIndex;
        uint randomRotationTextureIndex;
        uint sceneBufferIndex;
        uint ssaoBufferIndex;
        uint outputTextureIndex;
    };

    struct BoxBlurRenderResources
    {
        uint textureIndex;
        uint outputTextureIndex;
    };

    struct BloomExtractRenderResources
    {
        uint inputTextureIndex;
        uint outputTextureIndex;

        uint bloomBufferIndex;
    };

    // Having a float2 here to prevent having to flush the command queue (which is required if constant buffer is used to store texel size).
    struct BloomDownSampleRenderResources
    {
        uint inputTextureIndex;
        uint inputTextureMipLevel;
        uint outputTextureIndex;
        uint bloomPassIndex;
        float2 texelSize;
    };

     // Having a float2 here to prevent having to flush the command queue (which is required if constant buffer is used to store texel size).
    struct BloomUpSampleRenderResources
    {
        uint inputPreviousUpSampleSrvIndex;
        uint inputPreviousUpSampleMipLevel;
        uint inputCurrentDownSampleUavIndex;
        uint outputCurrentUpSampleMipIndex;
        uint bloomPassIndex;
        uint bloomBufferIndex;
        uint padding2;
        float2 texelSize;
    };
}
