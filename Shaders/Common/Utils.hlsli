#ifndef __UTILS_HLSLI__
#define __UTILS_HLSLI__

static const float MIN_FLOAT_VALUE = 0.00001f;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;
static const float INV_PI = 1.0f / PI;
static const float INV_TWO_PI = 1.0f / TWO_PI;
static const float INVALID_INDEX = 4294967295; // UINT_MAX;

float4 GetAlbedo(float2 textureCoords, uint albedoTextureIndex, uint albedoTextureSamplerIndex)
{
    if (albedoTextureIndex == INVALID_INDEX)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];

    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(albedoTextureSamplerIndex)];
    return albedoTexture.Sample(samplerState, textureCoords);
}

float3 GetNormal(float2 textureCoord, uint normalTextureIndex, uint normalTextureSamplerIndex, float3 normal, float3x3 tbnMatrix)
{
    float3 inputNormal = normal;

    if (normalTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(normalTextureSamplerIndex)];

        // Make the normal into a -1 to 1 range.
        normal = 2.0f * normalTexture.Sample(samplerState, textureCoord).xyz - float3(1.0f, 1.0f, 1.0f);
        normal = normalize(mul(normal, tbnMatrix));
        return normal;
    }
    
    return normalize(inputNormal);
}

float3 GetEmissive(float2 textureCoord, uint emissiveTextureIndex, uint emissiveTextureSamplerIndex)
{
    if (emissiveTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[NonUniformResourceIndex(emissiveTextureIndex)];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(emissiveTextureSamplerIndex)];

        return emissiveTexture.Sample(samplerState, textureCoord).xyz;
    }

    return float3(0.0f, 0.0f, 0.0f);
}

float GetAO(float2 textureCoord, uint aoTextureIndex, uint aoTextureSamplerIndex)
{
    if (aoTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> aoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(aoTextureIndex)];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(aoTextureSamplerIndex)];

        return aoTexture.Sample(samplerState, textureCoord).x;
    }

    return 1.0f;
}

float2 GetMetalRoughness(float2 textureCoord, uint metalRoughnessTextureIndex, uint metalRoughnessTextureSamplerIndex)
{
    if (metalRoughnessTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[NonUniformResourceIndex(metalRoughnessTextureIndex)];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(metalRoughnessTextureSamplerIndex)];

        return metalRoughnessTexture.Sample(samplerState, textureCoord).bg;
    }

    return float2(0.5f, 0.5f);
}

float3 GetSamplingVector(float2 pixelCoords, uint3 dispatchThreadID)
{
    // Convert pixelCoords into the range of -1 .. 1 and make sure y goes from top to bottom.
    float2 uv = 2.0f * float2(pixelCoords.x, 1.0f - pixelCoords.y) - float2(1.0f, 1.0f);

    float3 samplingVector = float3(0.0f, 0.0f, 0.0f);

    // Based on cube face 'index', choose a vector.
    switch (dispatchThreadID.z)
    {
    case 0:
        samplingVector = float3(1.0, uv.y, -uv.x);
        break;
    case 1:
        samplingVector = float3(-1.0, uv.y, uv.x);
        break;
    case 2:
        samplingVector = float3(uv.x, 1.0, -uv.y);
        break;
    case 3:
        samplingVector = float3(uv.x, -1.0, uv.y);
        break;
    case 4:
        samplingVector = float3(uv.x, uv.y, 1.0);
        break;
    case 5:
        samplingVector = float3(-uv.x, uv.y, -1.0);
        break;
    }

    samplingVector = normalize(samplingVector);

    return samplingVector;
}

float4 GenerateTangent(float3 normal)
{
    float3 tangent = cross(normal, float3(0.0f, 1.0f, 0.0));
    tangent = normalize(lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), tangent, step(MIN_FLOAT_VALUE, dot(tangent, tangent))));

    return float4(tangent, 1.0f);
}

#endif