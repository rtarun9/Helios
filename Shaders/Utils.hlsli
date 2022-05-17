#ifndef __UTILS_HLSLI__
#define __UTILS_HLSLI__

#include "BindlessRS.hlsli"

static const float MIN_FLOAT_VALUE = 0.00001f;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;
static const float INV_PI = 1.0f / PI;
static const float INV_TWO_PI = 1.0f / TWO_PI;

// Base reflectivity of surface.
static const float3 BASE_REFLECTIVITY = float3(0.04f, 0.04f, 0.04f);

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

float3 TangentSpaceToWorldSpace(float3 v, float3 n, float3 t, float3 s)
{
    return normalize(s * v.x + t * v.y + n * v.z);
}

float4 GenerateTangent(float3 n)
{
    float3 t = cross(n, float3(0.0f, 1.0f, 0.0));
    t = normalize(lerp(cross(n, float3(1.0f, 0.0f, 0.0f)), t, step(MIN_FLOAT_VALUE, dot(t, t))));
    
    return float4(t, 1.0f);
}

void GenerateBasisFromVector(float3 n, inout float3 t, inout float3 s)
{
    t = cross(n, float3(0.0f, 1.0f, 0.0));
    t = normalize(lerp(cross(n, float3(1.0f, 0.0f, 0.0f)), t, step(MIN_FLOAT_VALUE, dot(t, t))));
    s = normalize(cross(n, t));
}

// Point light attenuation based on Cem Yuskel's "Point Light Attenuation without Singularity Siggraph 2022 Talk".
float PointLightAttenuationWithoutSingularity(float distance, float radius)
{
    return 2.0f / (distance * distance + radius * radius + distance * sqrt(distance * distance + radius * radius));
}

// Punctual lights attenuation based on: https://google.github.io/filament/Filament.html#lighting/directlighting/punctuallights.
float PunctualLightAttenuation(float distance, float radius)
{
    float fallOffFactor = saturate(pow(1.0f - (pow(distance, 4) / pow(radius, 4)), 2));
    return fallOffFactor / max(distance * distance, 0.01f * 0.01f);
}

// Helpers for getting material values (albedo, metal roughness, etc).
float3 GetNormal(float3 inputNormal, float2 texCoord, float4 tangent, float3x3 modelMatrix, uint normalTextureIndex)
{
    if (normalTextureIndex)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];
        
        inputNormal = normalize(inputNormal);
        float3 normal = normalize(2.0f * normalTexture.Sample(anisotropicSampler, texCoord).xyz - float3(1.0f, 1.0f, 1.0f));

        tangent.xyz = normalize(tangent.xyz);
        
        float3 bitangent = normalize(cross(inputNormal, tangent.xyz)) * tangent.w;
        float3x3 tbn = float3x3(tangent.xyz, bitangent, inputNormal);
    
        normal = mul(normal, tbn);
        normal = normalize(mul(normal, modelMatrix));
        
        return normal;
    }
    else
    {
        return normalize(inputNormal);
    }
}

float3 GetAO(float2 texCoord, uint aoTextureIndex)
{
    if (aoTextureIndex)
    {
        Texture2D<float4> aoTexture = ResourceDescriptorHeap[aoTextureIndex];
        return aoTexture.Sample(linearWrapSampler, texCoord).xyz;
    }
    
    return float3(1.0f, 1.0f, 1.0f);
}

float3 GetEmissive(float2 texCoord, uint emissiveTextureIndex)
{
    if (emissiveTextureIndex)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[emissiveTextureIndex];
        return emissiveTexture.Sample(anisotropicSampler, texCoord).xyz;
    }
    
    return float3(0.0f, 0.0f, 0.0f);
}

float2 GetMetallicRoughnessFactor(float2 texCoord, uint metalRoughnessTextureIndex)
{
    if (metalRoughnessTextureIndex)
    {
        Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[metalRoughnessTextureIndex];
        float metallicFactor = metalRoughnessTexture.Sample(anisotropicSampler, texCoord).b;
        float roughnessFactor = metalRoughnessTexture.Sample(anisotropicSampler, texCoord).g;
        
        return float2(metallicFactor, roughnessFactor);
    }
    
    return float2(0.5f, 0.5f);
}

float4 GetAlbedo(float2 texCoord, uint albedoTextureIndex)
{
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];
    return albedoTexture.Sample(anisotropicSampler, texCoord);
}

#endif