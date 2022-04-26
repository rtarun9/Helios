#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
};

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

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

static const float MIN_FLOAT_VALUE = 0.00001f;
static const float PI = 3.14159265359;
static const uint LIGHT_COUNT = 1u;

// Fresnel effect : Amount of specular reflection based on the viewing angle to surface.
// F0 : Base reflectivity when view direction is perpendicular to the surface.
float3 FresnelSchlick(float3 viewDir, float3 halfWayDir, float3 F0)
{
    float cosTheta = max(dot(viewDir, halfWayDir), 0.0f);
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// Uses a slightly modified version (from: https://seblagarde.wordpress.com/2011/08/17/hello-world/) to take roughness into account.
float3 FresnelSchlick(float3 normal, float3 viewDir, float3 F0, float roughness)
{
    float cosTheta = max(dot(viewDir, normal), 0.0f);
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
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
    // Roughness is remapped to (roughness + 1) / 2 before squaring.
    // Source :https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
    
    roughness = (roughness + 1.0f) / 2.0f;
    float k = pow((roughness + 1), 2) / 8.0f;
    
    float nDotX = max(dot(normal, X), 0.0f);
    return nDotX / max(nDotX * (1.0f - k) + k, MIN_FLOAT_VALUE);
}

float SchlickGGXShadowing(float roughness, float3 normal, float3 viewDir, float3 lightDir)
{
    return SchlickBeckmannGS(roughness, normal, viewDir) * SchlickBeckmannGS(roughness, normal, lightDir);
}

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<MaterialData> materialCBuffer = ResourceDescriptorHeap[renderResource.materialCBufferIndex];
        
    ConstantBuffer<LightingData> lightCBuffer = ResourceDescriptorHeap[renderResource.lightCBufferIndex];
   
    Texture2D<float4> baseTexture = ResourceDescriptorHeap[renderResource.baseTextureIndex];
    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[renderResource.metalRoughnessTextureIndex];
    
    float3 normal = normalize(input.normal);
    
    float3 viewDir = normalize(lightCBuffer.cameraPosition.xyz - input.worldSpacePosition.xyz);

    float metallicFactor = metalRoughnessTexture.Sample(pointClampSampler, input.texCoord).x;
    float roughnessFactor = metalRoughnessTexture.Sample(pointClampSampler, input.texCoord).y;
    float3 albedo = baseTexture.Sample(pointClampSampler, input.texCoord).xyz;

    // Ignoring values in textures and using values from a constant buffer instead for testing.
    metallicFactor = materialCBuffer.metallicFactor;
    roughnessFactor = materialCBuffer.roughnessFactor;
    albedo = materialCBuffer.albedo;
    
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, float3(metallicFactor, metallicFactor, metallicFactor));
    
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Calculate irradiance due to each light source.
    for (uint i = 0; i < LIGHT_COUNT; ++i)
    {
        float3 lightColor = float3(1.0f, 1.0f, 1.0f);
        float3 pixelToLightDir = normalize(lightCBuffer.lightPosition - input.worldSpacePosition).xyz;
        float3 halfWayDir = normalize(viewDir + pixelToLightDir);

        float distance = length(pixelToLightDir);
        float attenuation = 1.0 / max((pow(distance, 2)), MIN_FLOAT_VALUE);
        
        float3 radiance = lightColor * attenuation;
        
        // Cook - Torrance BRDF Calculation.
        // Formula : f(cook-torrance) = DFG / (4(w0.n)(wi.n))
        float3 NDF = GGXNormalDistribution(normal, halfWayDir, roughnessFactor);
        float G = SchlickGGXShadowing(roughnessFactor, normal, viewDir, pixelToLightDir);
        float3 F = FresnelSchlick(viewDir, halfWayDir, F0);
        
        float3 specular = (NDF * G * F) / max(4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, pixelToLightDir), 0.0f), MIN_FLOAT_VALUE) + MIN_FLOAT_VALUE;
        
        float3 kS = F;
        float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), metallicFactor);
        
        float nDotL = max(dot(normal, pixelToLightDir), 0.0f);
        
        float3 lambertianDiffuse = materialCBuffer.albedo * kD / PI;
        
        float3 BRDF = lambertianDiffuse * kD + specular;
        
        Lo += (BRDF * radiance * nDotL);
    }
    
    // IBL Calculation.
    
    // For diffuse IBL.
    TextureCube<float4> irradianceMap = ResourceDescriptorHeap[renderResource.irradianceMap];
    float3 irradiance = irradianceMap.SampleLevel(linearWrapSampler, normal.xyz, 0.0f).xyz;
    
    float3 kS = FresnelSchlick(normal, viewDir, F0, materialCBuffer.roughnessFactor);
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;

    float3 diffuse = irradiance * materialCBuffer.albedo;
    float3 diffuseIBL = kD * diffuse;
    
    // For Specular IBL.
    TextureCube<float4> specularIrradianceMap = ResourceDescriptorHeap[renderResource.prefilterMap];
    Texture2D<float2> specularBRDF = ResourceDescriptorHeap[renderResource.brdfConvolutionLUTMap];
    
    // Get number of mip levels.
    uint specularTextureWidth, specularTextureHeight, levels;
    specularIrradianceMap.GetDimensions(0u, specularTextureWidth, specularTextureHeight, levels);

    float cosLO = max(dot(normal, viewDir), 0.0f);
    float3 LR = reflect(-viewDir, normal);
    
    float3 specularIrradiance = specularIrradianceMap.SampleLevel(linearClampSampler, LR, metallicFactor * (levels - 1.0f)).rgb;
    specularIrradiance = specularIrradianceMap.SampleLevel(linearClampSampler, LR, 0.0f).rgb;
    
    float2 specularBRDFLUT = specularBRDF.Sample(linearClampSampler, float2(saturate(dot(normal, viewDir)), roughnessFactor)).rg;

    float3 specularIBL = (F0 * specularBRDFLUT.x + specularBRDFLUT.y) * specularIrradiance;
    
    float3 outgoingLight = specularIBL + diffuseIBL + Lo;
    outgoingLight = diffuseIBL + Lo;
    
    return float4(outgoingLight, 1.0f);
}