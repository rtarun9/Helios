#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"
#include "BRDF.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    matrix lightSpaceMatrix : LIGHT_SPACE_MATRIX;
};

ConstantBuffer<DeferredLightingPassRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float2> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordsBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];

    ConstantBuffer<ShadowMappingBuffer> shadowMappingBuffer = ResourceDescriptorHeap[renderResource.shadowMappingBufferIndex];

    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xy, 0.0f, 1.0f);
    output.textureCoord = textureCoordsBuffer[vertexID];
    output.lightSpaceMatrix = shadowMappingBuffer.viewProjectionMatrix;

    return output;
}

// Reference : https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping.
float CalculateShadow(float4 lightSpaceWorldPosition, float nDotL, uint shadowDepthBufferIndex)
{
    // Do perspective divide
    float3 shadowPosition = lightSpaceWorldPosition.xyz / lightSpaceWorldPosition.w;
    
    // Convert x and y coord from [-1, 1] range to [0, 1].
    shadowPosition.x = shadowPosition.x * 0.5f + 0.5f;
    shadowPosition.y = shadowPosition.y * -0.5f + 0.5f;

    if (shadowPosition.z > 1.0f)
    {
        return 0.0f;
    }

    Texture2D<float4> shadowDepthBuffer = ResourceDescriptorHeap[shadowDepthBufferIndex];
    float closestDepth = shadowDepthBuffer.Sample(linearClampSampler, shadowPosition.xy).x;

    float bias = max(0.05f * (1.0f - nDotL), 0.005f);

    return shadowPosition.z  - bias > closestDepth ? 1.0f : 0.0f;
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

    // Will be used for calculated bias in shadow mapping as well (as it is highly dependent on nDotL).
    float nDotL = 0.0f;
    for (uint i = DIRECTIONAL_LIGHT_OFFSET; i < DIRECTIONAL_LIGHT_OFFSET + TOTAL_DIRECTIONAL_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(-lightBuffer.lightPosition[i].xyz);
        
        float3 brdf = BRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);

        float3 radiance = lightBuffer.lightColor[i].xyz;

        nDotL = saturate(dot(pixelToLightDirection, normal));
        lo += brdf *  radiance * nDotL;
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

    float4 lightSpacePosition = mul(float4(position.xyz, 1.0f), psInput.lightSpaceMatrix);

    float3 ambient = (diffuseIBL + specularIBL) * ao * (1.0f -  CalculateShadow(lightSpacePosition, nDotL, renderResource.shadowDepthTextureIndex));

    lo += emissive + ambient;

    return float4(lo, albedo.w);
}