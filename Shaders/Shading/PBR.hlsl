#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

static const float3 BASE_DIELECTRIC_REFLECTIVITY = float3(0.04f, 0.04f, 0.04f);

// Compute the ratio of reflected light vs how much it refracts.
// As the angle of incidence increases, this ratio increases as well (quickly approaching one when angle is 80+ degrees).
// f0 is the base reflectivity : the surface reflection at zero incidence. For non metals, it will just be a singular value in a float3 (v, v, v), but this is tinted for metals.
// Most dielectrics have a value of 0.04 as f0, but depending on how metallic a surface is it will be between 0.04 (metalness = 0) and the surface color (metalness = 1).
// cosTheta here is teh angle between the halfway vector and the view direction. If the angle is 0.0, then said ratio is 1, and the light will be brightest here.
// Also acts as the kS term (where kS + kD = 1, due to energy conservation).
float3 FresnelSchlickApproximation(float vDotH, float3 f0)
{
    return f0 + (1.0f - f0) * pow(clamp(1.0f - vDotH, 0.0f, 1.0f), 5.0f);
}

// Approximates the number of microfacts on the surface whose local normals are aligned with the half way vector. For light to reflect from the surface (diffuse or specular)
// and reach our camera, the normal and halfway vector have to be aligned. More rough a surface is, more chaotically aligned the surface normals will be, producing large and dim highlights, while very smooth surfaces
// will produce very sharp and bright highlights since majority of microfacet normals are aligned to half way vector.
float NormalDistributionGGX(float3 normal, float3 halfWayVector, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquare = alpha * alpha;

    float nDotH = saturate(dot(normal, halfWayVector));
    
    return alphaSquare / (max(PI * pow((nDotH * nDotH * (alphaSquare - 1.0f) + 1.0f), 2.0f), MIN_FLOAT_VALUE));
}

// Geometry function : approximates the number / relative surface area of the surface which is actually visible to us.
// If the surface is rough, several microfacets could overshadow and block others, because of which the light reaching us will be occluded.
// Using Smith's method, by changing the angle, we can approximate both self shadowing and geometry obstruction.
// Source :https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// if x is viewDirection, then we are calculating geometric obstruction, and if light direction, we are calculating self shadowing.
float SchlickBeckmannGS(float3 normal, float3 x, float roughness)
{
    float k = roughness / 2.0f;
    float nDotX = saturate(dot(normal, x));
    
    return nDotX / (max((nDotX * (1.0f - k) + k), MIN_FLOAT_VALUE));
}

// Smiths method is used for approximation of geomeetry (both self shadowing and geometry obstruction).
float SmithGGX(float3 normal, float3 viewDirection, float3 lightDirection, float roughness)
{
    return SchlickBeckmannGS(normal, viewDirection, roughness) * SchlickBeckmannGS(normal, lightDirection, roughness);    
}

ConstantBuffer<DeferredLightingPassRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float2> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordsBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];

    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xy, 0.0f, 1.0f);
    output.textureCoord = textureCoordsBuffer[vertexID];

    return output;
}

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];
    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    // Sample and extract data for the GBuffer's.
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResource.albedoGBufferIndex];
    Texture2D<float4> positionEmissiveTexture = ResourceDescriptorHeap[renderResource.positionEmissiveGBufferIndex];
    Texture2D<float4> normalEmissiveTexture = ResourceDescriptorHeap[renderResource.normalEmissiveGBufferIndex];
    Texture2D<float4> aoMetalRoughnessEmissiveTexture = ResourceDescriptorHeap[renderResource.aoMetalRoughnessEmissiveGBufferIndex];

    float4 albedo = albedoTexture.Sample(pointClampSampler, psInput.textureCoord);
    
    if (albedo.a < 0.9f)
    {
        discard;
    }

    float4 positionEmissive = positionEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);
    float4 normalEmissive = normalEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);
    float4 aoMetalRoughnessEmissive = aoMetalRoughnessEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);

    float3 position = positionEmissive.xyz;
    float3 normal = normalize(normalEmissive.xyz);

    float ao = aoMetalRoughnessEmissive.r;
    float metallicFactor = aoMetalRoughnessEmissive.g;
    float roughnessFactor = aoMetalRoughnessEmissive.b;

    float3 emissive = float3(positionEmissive.w, normalEmissive.w, aoMetalRoughnessEmissive.w);

    float3 viewDirection = normalize(sceneBuffer.cameraPosition.xyz - position);

    float3 lo = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(lightBuffer.lightPosition[i].xyz - position);
        float3 halfWayVector = normalize(viewDirection + pixelToLightDirection);
        float3 lightPosition = normalize(lightBuffer.lightPosition[i].xyz);

        float nDotV = saturate(dot(normal, viewDirection));
        float nDotH = saturate(dot(normal, halfWayVector));
        float nDotL = saturate(dot(normal, lightPosition));
    
        float3 f0 = lerp(BASE_DIELECTRIC_REFLECTIVITY, albedo.xyz, metallicFactor);
       
        float3 fresnel = FresnelSchlickApproximation(max(dot(viewDirection, halfWayVector), 0.0f), f0);
        float normalDistribution = NormalDistributionGGX(normal, halfWayVector, roughnessFactor);
        float geometryShadowing = SmithGGX(normal, viewDirection, lightBuffer.lightPosition[i].xyz, roughnessFactor);
        
        float3 specularBRDF = (normalDistribution * geometryShadowing * fresnel) / (max(4.0f * nDotV * nDotL, MIN_FLOAT_VALUE));

        float3 kS = fresnel;

        // Metals have kD as 0.0f, so more metallic a surface is, closes kS ~ 1 and kD ~ 0.
        float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - fresnel, float3(0.0f, 0.0f, 0.0f), metallicFactor);
        
        float3 diffuseBRDF = albedo.xyz / PI;

        float distance = length(pixelToLightDirection);
        float attenuation = 1.0f / (distance * distance);

        float3 radiance = lightBuffer.lightColor[i].xyz * attenuation;

        lo += (kD * diffuseBRDF + specularBRDF) *  radiance * nDotL;
    }

    return float4(lo, albedo.w);
}