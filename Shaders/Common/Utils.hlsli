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

float3 GetNormal(float2 textureCoord, uint normalTextureIndex, uint normalTextureSamplerIndex, float3 normal, float3 biTangent, float3 tangent, float3x3 tbnMatrix)
{
    float3 inputNormal = normal;

    if (normalTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(normalTextureSamplerIndex)];
        normal = 2.0f * normalTexture.Sample(samplerState, textureCoord).xyz - float3(1.0f, 1.0f, 1.0f);
        normal = normalize(mul(normal, tbnMatrix));
        return normal;
    }
    
    return normalize(inputNormal);
}

float4 GenerateTangent(float3 normal)
{
    float3 tangent = cross(normal, float3(0.0f, 1.0f, 0.0));
    tangent = normalize(lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), tangent, step(MIN_FLOAT_VALUE, dot(tangent, tangent))));

    return float4(tangent, 1.0f);
}

// Punctual lights attenuation based on: https://google.github.io/filament/Filament.html#lighting/directlighting/punctuallights.
float GetSquareFalloffAttenuation(float3 pixelToLightDirection, float radius) 
{
    float lightInvRadius = 1.0f / radius;
    float distanceSquare = dot(pixelToLightDirection, pixelToLightDirection);
    float factor = distanceSquare * lightInvRadius * lightInvRadius;
    float smoothFactor = max(1.0 - factor * factor, 0.0);
    return (smoothFactor * smoothFactor) / max(distanceSquare, 1e-5);
}

#endif