// clang-format off
#pragma once

// To be able to share Constant Buffer Types (Structs) between HLSL and C++, a few defines are set here.
// Define the Structs in HLSLS syntax, and with this defines the C++ application can also use the structs.
#ifdef __cplusplus

#define float4 math::XMFLOAT4
#define float3 math::XMFLOAT3
#define float2 math::XMFLOAT2

#define uint uint32_t

// Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp
// code side.
#define float4x4 math::XMMATRIX

#define ConstantBufferStruct struct alignas(256)

#else
// if HLSL

#define ConstantBufferStruct struct

#endif

namespace interlop
{
    ConstantBufferStruct SceneBuffer
    {
        float4x4 viewMatrix;
        float4x4 projectionMatrix;
        float4x4 inverseViewMatrix;
        float4x4 viewProjectionMatrix;
    };

    ConstantBufferStruct TransformBuffer
    {
        float4x4 modelMatrix;
        float4x4 inverseModelMatrix;
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

    
    // There is a max limit to the number of lights in the engine.
    // Index 0 of both the light buffer will be
    // reserved for directional light.

    static const uint MAX_LIGHTS = 10;

    // Light resources that are used while instanced rendering.
    ConstantBufferStruct LightInstancedRenderingBuffer
    {
        float4x4 modelMatrix[MAX_LIGHTS];
    };

    // Lights resources / properties for all lights in the scene.
    ConstantBufferStruct LightBuffer
    {
        // Note : lightPosition essentially stores the light direction if the type is directional light.
        // The shader can differentiate between directional and point lights based on the 'w' value. If 1 (i.e it is a position), light is a point light, while
        // if it is zero, then it is a light direction.
        // Light intensity is not automatically multiplied to the light color on the C++ side, shading shader need to manually multiply them.
        float4 lightPosition[MAX_LIGHTS];
        float4 viewSpaceLightPosition[MAX_LIGHTS];

        float4 lightColor[MAX_LIGHTS];
        // float4 because of struct packing (16byte alignment).
        // radiusIntensity[0] stores the radius, while index 1 stores the intensity.
        float4 radiusIntensity[MAX_LIGHTS];
        
        uint numberOfLights;
    };

    ConstantBufferStruct PostProcessingBuffer
    {
        uint debugShowSSAOTexture;
        float4 padding;
    };

    // Note : By using the values in this buffer, the PBR renderer will most likely 'break' and become physically inaccurate.
    // These are used for debugging and testing purposes only.
    ConstantBufferStruct MaterialBuffer
    {
        float roughnessFactor;
        float metallicFactor;
        float emissiveFactor;
        float padding;
        float3 albedoColor;
        float padding2;
    };

    ConstantBufferStruct ShadowBuffer
    {
        float4x4 lightViewProjectionMatrix;
        float backOffDistance;
        float extents;
        float nearPlane;
        float farPlane;
    };

    ConstantBufferStruct SSAOBuffer
    {
        float4 sampleVectors[128];
        float radius;
        float bias;
        uint sampleVectorCount;
        float padding;
        float noiseTextureWidth;
        float noiseTextureHeight;
        float screenWidth;
        float screenHeight;
    };

} // namespace interlop
