#pragma once

static const float MIN_FLOAT_VALUE = 0.00001f;
static const float EPSILON = 1.0e-4;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;
static const float INV_PI = 1.0f / PI;
static const float INV_TWO_PI = 1.0f / TWO_PI;
static const float INVALID_INDEX = 4294967295; // UINT32_MAX;

float4 getAlbedo(const float2 textureCoords, const uint albedoTextureIndex, const uint albedoTextureSamplerIndex)
{
    if (albedoTextureIndex == INVALID_INDEX)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];

    SamplerState samplerState = SamplerDescriptorHeap[albedoTextureSamplerIndex];
    return albedoTexture.Sample(samplerState, textureCoords);
}