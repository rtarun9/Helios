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
        float4x4 viewProjectionMatrix;
    };

    ConstantBufferStruct TransformBuffer
    {
        float4x4 modelMatrix;
        float4x4 inverseModelMatrix;
        float4x4 inverseModelViewMatrix;
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

    
    // Number of lights is currently fixed in the engine.
    // NOTE : Directional lights are placed after point lights
    static const uint POINT_LIGHT_OFFSET = 0; 
    static const uint TOTAL_POINT_LIGHTS = 1;
    
    static const uint DIRECTIONAL_LIGHT_OFFSET = TOTAL_POINT_LIGHTS;
    static const uint TOTAL_DIRECTIONAL_LIGHTS = 1;
    
    // The light data (for all types) will be stored in a single constant buffer for simplicity. Subject to change.
    static const uint TOTAL_LIGHTS = TOTAL_DIRECTIONAL_LIGHTS + TOTAL_POINT_LIGHTS;

    // Light resources that are used while instanced rendering.
    ConstantBufferStruct LightInstancedRenderingBuffer
    {
        float4x4 modelMatrix[TOTAL_POINT_LIGHTS];
    };

    // Lights resources / properties for all lights in the scene.
    ConstantBufferStruct LightBuffer
    {
        // Note : lightPosition essentially stores the light direction if the type is directional light.
        // The shader can differentiate between directional and point lights based on the 'w' value. If 1 (i.e it is a position), light is a point light, while
        // if it is zero, then it is a light direction.
        // Light intensity is not automatically multiplied to the light color on the C++ side, shading shader need to manually multiply them.
        float4 lightPosition[TOTAL_LIGHTS];
        float4 viewSpaceLightPosition[TOTAL_LIGHTS];

        float4 lightColor[TOTAL_LIGHTS];
        // float4 because of struct packing (16byte alignment).
        // radiusIntensity[0] stores the radius, while index 1 stores the intensity.
        float4 radiusIntensity[TOTAL_LIGHTS];
    };
} // namespace interlop
