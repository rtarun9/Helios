#ifndef __BINDLESS_RS_HLSLI__
#define __BINDLESS_RS_HLSLI__

#ifndef __cplusplus

#define BindlessRootSignature           \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
    "RootConstants(b0, num32BitConstants=64, visibility = SHADER_VISIBILITY_ALL)," \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)," \
    "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)," \
    "StaticSampler(s2, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), " \
    "StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP), " \
    "StaticSampler(s4, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)," \
    "StaticSampler(s5, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)," \
    "StaticSampler(s6, filter = FILTER_MIN_MAG_POINT_MIP_LINEAR, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), " \
    "StaticSampler(s7, filter = FILTER_MIN_MAG_POINT_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP), " \
    "StaticSampler(s8, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)"

// Samplers
SamplerState pointClampSampler : register(s0);
SamplerState pointWrapSampler : register(s1);
SamplerState linearWrapSampler : register(s2);
SamplerState linearClampSampler: register(s3);
SamplerState minMapLinearMipPointClampSampler : register(s4);
SamplerState minMapLinearMipPointWrapSampler : register(s5);
SamplerState minMapPointMipLinearClampSampler : register(s6);
SamplerState minMapPointMipLinearWrapSampler : register(s7);
SamplerState anisotropicSampler : register(s8);

#else // ifdef __cplusplus
#define uint uint32_t
#endif

// All *RenderResources structs are placed here to prevent having them in multiple places.
struct TestRenderResources
{
    uint positionBufferIndex;
    uint textureBufferIndex;
    uint normalBufferIndex;
    
    uint mvpCBufferIndex;
    
    uint lightCBufferIndex;
    uint textureIndex;
};

struct PBRRenderResources
{
    uint positionBufferIndex;
    uint textureBufferIndex;
    uint normalBufferIndex;
    uint tangetBufferIndex;
    
    uint mvpCBufferIndex;
    
    uint materialCBufferIndex;
    uint lightCBufferIndex;
    
    uint albedoTextureIndex;
    uint metalRoughnessTextureIndex;
    uint emissiveTextureIndex;
    uint normalTextureIndex;
    uint aoTextureIndex;
    
    uint irradianceMap;
    uint prefilterMap;
    uint brdfConvolutionLUTMap;
};

struct RenderTargetRenderResources
{
    uint positionBufferIndex;
    uint textureBufferIndex;
    uint textureIndex;
    uint renderTargetSettingsCBufferIndex;
};

struct LightRenderResources
{
    uint positionBufferIndex;
    uint mvpCBufferIndex;
};

struct SkyBoxRenderResources
{
    uint positionBufferIndex;
    uint mvpCBufferIndex;
    uint textureIndex;
};

struct CubeFromEquirectRenderResources
{
    uint textureIndex;
    uint outputTextureIndex;
};

struct CubeMapConvolutionRenderResources
{
    uint textureCubeMapIndex;
    uint outputIrradianceMapIndex;
};

struct PreFilterCubeMapRenderResources
{
    uint textureCubeMapIndex;
    uint outputPreFilteredCubeMapIndex;
    uint mipLevel;
};

struct BRDFConvolutionRenderResources
{
    uint lutTextureIndex;
};

#endif