#ifndef __IBL_HLSLI__
#define __IBL_HLSLI__

#include "BindlessRS.hlsli"
#include "Utils.hlsli"
#include "BRDF.hlsli"

float3 DiffuseIBL(float3 normal, float3 albedo, float roughness, float metallic, float3 viewDirection, uint irradianceMapTextureIndex)
{
    TextureCube<float4> irradianceMap = ResourceDescriptorHeap[irradianceMapTextureIndex];
    float3 irradiance = irradianceMap.SampleLevel(linearWrapSampler, normal.xyz, 0.0f).xyz;

    float cosTheta = saturate(dot(normal, viewDirection));

    float3 kS = FresnelSchlick(cosTheta, BASE_REFLECTIVITY, roughness);
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), metallic);

    return irradiance * albedo * kD;
}

float3 SpecularIBL(float3 normal, float3 albedo, float3 viewDirection, float roughness, float metallic, uint prefilterMapTextureIndex, uint brdfLUTTextureIndex)
{
    TextureCube<float4> specularIrradianceMap = ResourceDescriptorHeap[prefilterMapTextureIndex];
    Texture2D<float2> brdfLut = ResourceDescriptorHeap[brdfLUTTextureIndex];
    
    float3 lr = reflect(-viewDirection, normal);
    float cosLo = saturate(dot(viewDirection, normal));
    
    float3 f0 = lerp(BASE_REFLECTIVITY, albedo, metallic);
    
    uint specularTextureWidth, specularTextureHeight, levels;
    specularIrradianceMap.GetDimensions(0u, specularTextureWidth, specularTextureHeight, levels);

    float3 specularIrradiance = specularIrradianceMap.SampleLevel(linearWrapSampler, lr, roughness * 6.0f).rgb;
    
    float2 specularBRDFLUT = brdfLut.Sample(pointWrapSampler, float2(cosLo, roughness), 0.0f).rg;

    return specularIrradiance * (f0 * specularBRDFLUT.x + specularBRDFLUT.y);
}
#endif