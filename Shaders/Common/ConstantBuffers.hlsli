#ifndef __CONSTANT_BUFFERS_HLSLI__
#define __CONSTANT_BUFFERS_HLSLI__

#ifdef __cplusplus

#define float4 DirectX::XMFLOAT4
#define float3 DirectX::XMFLOAT3
#define float2 DirectX::XMFLOAT2

#define uint uint32_t
#define uint2 uint32_t 

// Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp code side. 
#define float4x4 DirectX::XMMATRIX

#define ConstantBufferStruct struct alignas(256)

#else //if HLSL

#define ConstantBufferStruct struct 

#endif

// Constants that are also shared between hlsl and C++.
// NOTE : Directional lights are placed after point lights
static const uint POINT_LIGHT_OFFSET = 0; 
static const uint TOTAL_POINT_LIGHTS = 200;

static const uint DIRECTIONAL_LIGHT_OFFSET = TOTAL_POINT_LIGHTS;
static const uint TOTAL_DIRECTIONAL_LIGHTS = 1;

// The light data (for all types) will be stored in a single constant buffer for simplicity. Subject to change.
static const uint TOTAL_LIGHTS = TOTAL_DIRECTIONAL_LIGHTS + TOTAL_POINT_LIGHTS;

// Hold all Constant Buffer struct's in a common shared place (shared between C++ and hlsl).

ConstantBufferStruct LUTCBuffer
{
    float lutIndex;
};

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
};

ConstantBufferStruct SceneBuffer
{
    float3 cameraPosition;
    float3 cameraTarget;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

ConstantBufferStruct LightBuffer
{
    // Note : lightPosition essentially stores the light direction if the type is directional light.
    // The shader can differentiate between directional and point lights based on the 'w' value. If 1 (i.e it is a position), light is a point light, while
    // if it is zero, then it is a light direction.
    float4 lightPosition[TOTAL_LIGHTS];
    float4 lightColor[TOTAL_LIGHTS];
    float radius[TOTAL_LIGHTS];
};

ConstantBufferStruct MipMapGenerationBuffer
{
    bool isSRGB;

    // 1.0f  / outputDimension.size
    float2 texelSize;
};

#endif