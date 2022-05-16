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
static const uint TOTAL_POINT_LIGHTS = 5;
static const uint TOTAL_DIRECTIONAL_LIGHTS = 1;
static const uint TOTAL_LIGHTS = TOTAL_DIRECTIONAL_LIGHTS + TOTAL_POINT_LIGHTS;

// Hold all Constant Buffer struct's in a common shared place (shared between C++ and hlsl)

ConstantBufferStruct LUTCBuffer
{
    float lutIndex;
};

ConstantBufferStruct TransformData
{
    float4x4 modelMatrix;
    float4x4 inverseModelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

ConstantBufferStruct PointLightData
{
    float4 lightPosition[TOTAL_POINT_LIGHTS];
    float4 lightColor[TOTAL_POINT_LIGHTS];
    float radius[TOTAL_POINT_LIGHTS];
};

ConstantBufferStruct DirectionalLightData
{
    float4 lightDirection[TOTAL_DIRECTIONAL_LIGHTS];
    float4 lightColor[TOTAL_DIRECTIONAL_LIGHTS];
};

ConstantBufferStruct RenderTargetSettings
{
    float exposure;
};

ConstantBufferStruct CameraData
{
    float3 cameraPosition;
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