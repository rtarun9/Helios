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
static const uint TOTAL_POINT_LIGHTS = 1;
static const uint TOTAL_DIRECTIONAL_LIGHTS = 1;

// The light data (for all types) will be stored in a single constant buffer for simplicity. Subject to change.
static const uint TOTAL_LIGHTS = TOTAL_DIRECTIONAL_LIGHTS + TOTAL_POINT_LIGHTS;

// Hold all Constant Buffer struct's in a common shared place (shared between C++ and hlsl).

ConstantBufferStruct LUTCBuffer
{
    float lutIndex;
};

ConstantBufferStruct TransformBuffer
{
    float4x4 modelMatrix;
    float4x4 inverseModelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

ConstantBufferStruct ShadowMappingData
{
    float4x4 lightViewMatrix;
    float4x4 lightProjectionMatrix;
};

ConstantBufferStruct LightData
{
    // Note : lightPosition essentially stores the light direction if the type is directional light.
    float4 lightPosition[TOTAL_LIGHTS];
    float4 lightColor[TOTAL_LIGHTS];
    float radius[TOTAL_LIGHTS];
};

ConstantBufferStruct RenderTargetSettings
{
    float exposure;
};

ConstantBufferStruct CameraData
{
    float3 cameraPosition;
    float4x4 viewMatrix;
};

ConstantBufferStruct MaterialData
{
    float3 albedo;
    float metallicFactor;
    float roughnessFactor;
    float ao;
    float2 padding;
    float3 emissive;
};

#endif