#include "PBRRS.hlsli"
#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
};

SamplerState clampSampler : register(s0, space1);
SamplerState wrapSampler : register(s1, space1);

struct LightingData
{
    float4 lightPosition;
    float4 cameraPosition;
};

struct MaterialData
{
    float3 albedo;
    float metallicFactor;
    float roughnessFactor;
    float ao;
    float2 padding;
};

ConstantBuffer<MaterialData> materialCBuffer : register(b0, space1);
ConstantBuffer<LightingData> lightCBuffer : register(b1, space1);

Texture2D baseTexture : register(t0, space1);
Texture2D metalRoughnessTexture : register(t1, space1);

static const float GAMMA_CORRECTION = 0.454545455f;

static const float MIN_FLOAT_VALUE = 0.0000001f;
static const float PI = 3.14159265f;

// Fresnel effect : Amount of specular reflection based on the viewing angle to surface.
// F0 : Base reflectivity when view direction is perpendicular to the surface.
float3 FresnelSchlick(float3 viewDir, float3 halfWayDir, float3 F0)
{
    float cosTheta = max(dot(viewDir, halfWayDir), 0.0f);
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5);
}

// PBR Shading model used : Cook Torrence model.

// Normal distrubution function using GGX / Trowbridge - Reitz model.
// Describe how microfacets are distributed accorded to roughness (the amount of microfacets aligned to half way vector).
// Formula : (alpha^2) / pi * ((N.H)^2.(alpha^2 - 1) + 1)^2.
float GGXNormalDistribution(float3 normal, float3 halfWayDir, float roughness)
{
    float alpha = pow(roughness, 2);
    float alphaSquare = pow(alpha, 2);

    float nDotH = max(dot(normal, halfWayDir), 0.0f);
    return alphaSquare / max((PI * pow(pow(nDotH, 2) * (alphaSquare - 1.0f) + 1.0f, 2)), MIN_FLOAT_VALUE);
}

// Geometry shadowing function used : Schlick - GGX.
// Describes self shadowing property of surface.
// Schlick - Beckmann function will be used to calculate geometry shadowing and the result will be used
// with Schlick - GGX to find self - shadowing and geometry obstruction.

// Schlick Beckmann Geometry Shadowing function.
// Formula : N . X / (N . X) * (1 - k) + k, where k = roughness / 2, X = viewDir for geometry obstruction or lightDir for self shadowing.
float SchlickBeckmannGS(float roughness, float3 normal, float3 X)
{
    float k = roughness / 2;
    
    float nDotX = max(dot(normal, X), 0.0f);
    return nDotX / max(nDotX * (1 - k) + k, MIN_FLOAT_VALUE);
}

float SchlickGGXShadowing(float roughness, float3 normal, float3 viewDir, float3 lightDir)
{
    return SchlickBeckmannGS(roughness, normal, viewDir) * SchlickBeckmannGS(roughness, normal, lightDir);
}

[RootSignature(PBRRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    float3 lightColor = float3(1.0f, 1.0f, 1.0f); 

    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(lightCBuffer.cameraPosition - input.worldSpacePosition).xyz;

    float3 pixelToLightDir = normalize(lightCBuffer.lightPosition - input.worldSpacePosition).xyz;
    float3 halfWayDir = normalize(viewDir + pixelToLightDir);

    float distance = length(pixelToLightDir);
    float attenuation = 1.0 / max((pow(distance, 2)), MIN_FLOAT_VALUE);

    float metallicFactor = metalRoughnessTexture.Sample(clampSampler, input.texCoord).x;
    float roughnessFactor = metalRoughnessTexture.Sample(clampSampler, input.texCoord).y;
    float3 albedo = baseTexture.Sample(clampSampler, input.texCoord).xyz;

    #if 1
    metallicFactor = materialCBuffer.metallicFactor;
    roughnessFactor = materialCBuffer.roughnessFactor;
    albedo = materialCBuffer.albedo;
    #endif 

    // Rendering equation (or reflectance equation) for PBR Calculation.
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, float3(metallicFactor, metallicFactor, metallicFactor));
    
    float3 kS = FresnelSchlick(viewDir, halfWayDir, F0);
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= (1.0f - metallicFactor);

    float3 lambertianDiffuse = albedo / PI;

    // Cook - Torrance BRDF Calculation.
    // Formula : f(cook-torrance) = DFG / (4(w0.n)(wi.n))
    float3 NDF = GGXNormalDistribution(normal, halfWayDir, roughnessFactor);
    float G = SchlickGGXShadowing(roughnessFactor, normal, viewDir, pixelToLightDir);
    float3 F = kS;

    float3 specular = (NDF * G * F) / max(4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, pixelToLightDir), 0.0f), MIN_FLOAT_VALUE);
    float3 BRDF = lambertianDiffuse * kD + specular;

    float nDotL = max(dot(normal, pixelToLightDir), 0.0f);
    float3 outgoingLight = BRDF * lightColor * nDotL;

    return float4(pow(outgoingLight, GAMMA_CORRECTION), 1.0f);
}