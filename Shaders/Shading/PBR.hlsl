#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"
#include "BRDF.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

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

    TextureCube<float4> irradianceMap = ResourceDescriptorHeap[renderResource.irradianceMapIndex];
    TextureCube<float4> prefilterMap = ResourceDescriptorHeap[renderResource.prefilterMapIndex];
    Texture2D<float2> brdfLutMap = ResourceDescriptorHeap[renderResource.brdfLutIndex];

    float4 albedo = albedoTexture.Sample(pointClampSampler, psInput.textureCoord);
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

    // Reflectance equation for reference.
    // lo(x, v) = le(x, v) + integral(over hemisphere centered at x)(fr(x, l, v, roughness) * li(x, l) * (l.n)dl
    // x is the pixel position
    // v is the view direction
    // l is the light vector
    // lo is the irradiance, while li is the light radiance
    // fr is the brdf term.

    float3 lo = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(lightBuffer.lightPosition[i].xyz - position);

        float3 brdf = BRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);

        float distance = length(lightBuffer.lightPosition[i].xyz - position);
        float attenuation = 1.0f / (distance * distance);

        float3 radiance = lightBuffer.lightColor[i].xyz * attenuation;

        lo += brdf *  radiance * saturate(dot(pixelToLightDirection, normal));
    }

    for (uint i = DIRECTIONAL_LIGHT_OFFSET; i < DIRECTIONAL_LIGHT_OFFSET + TOTAL_DIRECTIONAL_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(-lightBuffer.lightPosition[i].xyz);
        
        float3 brdf = BRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);

        float3 radiance = lightBuffer.lightColor[i].xyz;

        lo += brdf *  radiance * saturate(dot(pixelToLightDirection, normal));
    }

    // Calculate ambient lighting from irradiance map.
    float3 f0 = lerp(BASE_DIELECTRIC_REFLECTIVITY, albedo.xyz, metallicFactor);
    float3 kS = FresnelSchlickApproximation(f0, saturate(dot(viewDirection, normal)), roughnessFactor);
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), metallicFactor);
    float3 irradiance = irradianceMap.Sample(linearWrapSampler, normal).rgb;
    float3 diffuseIBL = kD * irradiance * albedo.xyz;

    // Calculate specular IBL.
    float3 lr = reflect(-viewDirection, normal);
    float cosLo = saturate(dot(viewDirection, normal));
    
    uint prefilterTextureWidth, prefilterTextureHeight, levels;
    prefilterMap.GetDimensions(0u, prefilterTextureWidth, prefilterTextureHeight, levels);

    float3 specularPrefilter = prefilterMap.SampleLevel(linearWrapSampler, lr, roughnessFactor * 6.0f).rgb;
    float2 brdfLut = brdfLutMap.Sample(pointWrapSampler, float2(cosLo, roughnessFactor), 0.0f).rg;

    float3 specularIBL = specularPrefilter * (f0 * brdfLut.x + brdfLut.y);

    float3 ambient = (diffuseIBL + specularIBL) * ao;

    lo += emissive + ambient;

    return float4(lo, albedo.w);
}