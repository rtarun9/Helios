#ifndef __BRDF_HLSLI__
#define __BRDF_HLSLI__

#include "Utils.hlsli"

// Returns the ratio between how much surface reflects vs how much it refracts light.
// f0 : surface reflection at zero incidence.
float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// Uses a slightly modified version (from: https://seblagarde.wordpress.com/2011/08/17/hello-world/) to take roughness into account.
float3 FresnelSchlick(float3 cosTheta, float3 f0, float roughness)
{
    return f0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), f0) - f0) * pow(1.0 - cosTheta, 5.0);
}

// Approximates amount of surface microfacets aligned to half way vector, which is influenced by the roughness.
// Formula used for reference : (alpha^2)/ pi * ((n.h)^2.(alpha^2 - 1) + 1)^2.
float GGXNormalDistribution(float3 normal, float3 halfWayVector, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquare = alpha * alpha;
    
    float nDotH = saturate(dot(normal, halfWayVector));
  
    return alphaSquare / (max(PI * pow((nDotH * nDotH * (alphaSquare - 1.0f) + 1.0f), 2.0f), MIN_FLOAT_VALUE));
}

// Describes self shadowing property of microfacets.
// Using a combination f16tof32 the Schlick-Beckmann approximation and the GGX approximation.
// Formula used : n.x/(n.x * (1.0f - k) + k), where k = roughness / 2.
// Source :https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float SchlickBeckmannGS(float roughness, float3 normal, float3 x)
{
    float k = roughness / 2.0f;
    float nDotX = saturate(dot(normal, x));
    
    return nDotX / (max((nDotX * (1.0f - k) + k), MIN_FLOAT_VALUE));
}

// Smiths method is used for approximation of geomeetry (both self shadowing and geometry obstruction).
float SchlickGGX(float roughness, float3 normal, float3 viewDirection, float3 lightDirection)
{
    return SchlickBeckmannGS(roughness, normal, viewDirection) * SchlickBeckmannGS(roughness, normal, lightDirection);    
}

// Cook Torrence BRDF formula used : 
// kD * c / PI + NGF / (4 * w0.n * wi.n)).
// Here, wi : incoming light direction, wo = outgoing light direction
float3 CookTorrenceBRDF(float3 normal, float3 viewDirection, float3 lightDirection, float3 albedo, float metallic, float roughness)
{
    float3 halfWayVector = normalize(lightDirection + viewDirection);
    
    float3 f0 = lerp(BaseReflectivity, albedo, metallic);
    float hDotV = saturate(dot(halfWayVector, viewDirection));
    float nDotL = saturate(dot(normal, lightDirection));
    float nDotV = saturate(dot(normal, viewDirection));
    
    // Specular term of Cook Torrence BRDF.
    float3 fresnelEffect = FresnelSchlick(hDotV, f0);
    float normalDistribution = GGXNormalDistribution(normal, halfWayVector, roughness);
    float geometryShadowing = SchlickGGX(roughness, normal, viewDirection, lightDirection);
    
    float3 specularBRDF = (normalDistribution * fresnelEffect * geometryShadowing) / (max((4.0f * nDotL * nDotV), MIN_FLOAT_VALUE));

    // Diffuse term of Cook Torrence BRDF.
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - fresnelEffect, float3(0.0f, 0.0f, 0.0f), metallic);
    float3 diffuseBRDF = albedo / PI;
    
    return kD * diffuseBRDF + specularBRDF;
}

#endif