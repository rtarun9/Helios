#ifndef __CONSTANT_BUFFERS_HLSLI__
#define __CONSTANT_BUFFERS_HLSLI__

#ifdef __cplusplus

#define float4 DirectX::XMFLOAT4
#define float3 DirectX::XMFLOAT3
#define float2 DirectX::XMFLOAT2

#define uint uint32_t

// Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp code side. 
#define float4x4 DirectX::XMMATRIX

#define ConstantBufferStruct struct alignas(256)

#else //if HLSL

#define ConstantBufferStruct struct 

#endif

// Constants that are also shared between hlsl and C++.
// NOTE : Directional lights are placed after point lights
static const uint POINT_LIGHT_OFFSET = 0; 
static const uint TOTAL_POINT_LIGHTS = 1;

static const uint DIRECTIONAL_LIGHT_OFFSET = TOTAL_POINT_LIGHTS;
static const uint TOTAL_DIRECTIONAL_LIGHTS = 1;

// The light data (for all types) will be stored in a single constant buffer for simplicity. Subject to change.
static const uint TOTAL_LIGHTS = TOTAL_DIRECTIONAL_LIGHTS + TOTAL_POINT_LIGHTS;

// Hold all Constant Buffer struct's in a common shared place (shared between C++ and hlsl).

// Light resources that are used while instanced rendering.
ConstantBufferStruct InstanceLightBuffer
{
    float4x4 modelMatrix[TOTAL_POINT_LIGHTS];
};

ConstantBufferStruct TransformBuffer
{
    float4x4 modelMatrix;
    float4x4 inverseModelMatrix;
};

ConstantBufferStruct PostProcessBuffer
{
    float exposure;
    float3 padding;
};

ConstantBufferStruct SceneBuffer
{
    float3 cameraPosition;
    float padding;
    float3 cameraTarget;
    float padding2;
    float4x4 viewProjectionMatrix;
};

ConstantBufferStruct ShadowMappingBuffer
{
    float4x4 viewProjectionMatrix;
    float backOffDistance;
    float extents;
    float nearPlane;
    float farPlane;
};

ConstantBufferStruct LightBuffer
{
    // Note : lightPosition essentially stores the light direction if the type is directional light.
    // The shader can differentiate between directional and point lights based on the 'w' value. If 1 (i.e it is a position), light is a point light, while
    // if it is zero, then it is a light direction.
    // Light intensity is not automatically multiplied to the light color on the C++ side, shading shaders need to manually multiply them.
    float4 lightPosition[TOTAL_LIGHTS];
    float4 lightColor[TOTAL_LIGHTS];
    // float4 because of struct packing (16byte alignment).
    // radiusIntensity[0] stores the radius, while index 1 stores the intensity.
    float4 radiusIntensity[TOTAL_LIGHTS];
};

enum class BloomShaderUsage : int
{
    Downsample,
    FirstDownsample,
    Upsample,
    PreFilter,
    GaussianBlurHorizontal,
    GaussianBlurVertical,
    Other
};

// threshHoldParams = float2(threshHoldValue, knee).
ConstantBufferStruct BloomBuffer
{
    BloomShaderUsage shaderUsage;
    
    float3 padding;
    
    float2 threshHoldParams;
    
    float2 padding2;
    
    float2 texelSize;
};

enum class TextureDimensionType
{
    HeightWidthEven,
    HeightEvenWidthOdd,
    HeightOddWidthEven,
    HeightWidthOdd
};


ConstantBufferStruct MipMapGenerationBuffer
{
    int isSRGB;

    uint sourceMipLevel;

    // 1.0f  / outputDimension.size
    float2 texelSize;

    uint numberMipLevels;

    TextureDimensionType dimensionType;
};

#endif