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
} // namespace interlop
