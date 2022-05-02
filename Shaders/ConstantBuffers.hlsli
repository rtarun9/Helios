#ifndef __CONSTANT_BUFFERS_HLSLI__
#define __CONSTANT_BUFFERS_HLSLI__

#ifdef __cplusplus

#define float4 DirectX::XMFLOAT4
#define float3 DirectX::XMFLOAT3
#define float2 DirectX::XMFLOAT2

// Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp code side. 
#define float4x4 DirectX::XMMATRIX

#define ConstantBufferStruct struct alignas(256)

#else //if HLSL

#define ConstantBufferStruct struct 

#endif

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

ConstantBufferStruct RenderTargetSettings
{
    float exposure;
};

ConstantBufferStruct LightingData
{
    float4 lightPosition;
    float4 cameraPosition;
};

ConstantBufferStruct MaterialData
{
    float3 albedo;
    float metallicFactor;
    float roughnessFactor;
    float ao;
    float2 padding;
};

#endif