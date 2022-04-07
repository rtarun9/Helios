#define BindlessRootSignature           \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
    "RootConstants(b0, num32BitConstants=64, visibility = SHADER_VISIBILITY_ALL)," \
    "StaticSampler(s0, space = 1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)," \
    "StaticSampler(s1, space = 1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)" 

// Samplers
SamplerState pointClampSampler : register(s0, space1);
SamplerState pointWrapSampler : register(s1, space1);

// All *RenderResources structs are placed here to prevent having them in multiple placed (VS and PS shader).
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
    
    uint mvpCBufferIndex;
    
    uint materialCBufferIndex;
    uint lightCBufferIndex;
    
    uint baseTextureIndex;
    uint metalRoughnessTextureIndex;
};

struct RenderTargetRenderResources
{
    uint positionBufferIndex;
    uint textureBufferIndex;
    uint textureIndex;
};

struct LightRenderResources
{
    uint positionBufferIndex;
    uint mvpCBufferIndex;
};