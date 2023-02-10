// clang-format off 
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/renderResources.hlsli"
#include "Utils.hlsli"

#include "Shading/BRDF.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<interlop::PBRRenderResources> renderResources : register(b0);


[RootSignature(BindlessRootSignature)] 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f),
                                               float3(-1.0f, -3.0f, 0.0f)};

    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[vertexID], 1.0f);
    output.textureCoord = output.position.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return output;
}

[RootSignature(BindlessRootSignature)] 
float4 PsMain(VSOutput psInput) : SV_Target
{
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    // Sample and extract data for the GBuffer's.
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResources.albedoGBufferIndex];
    Texture2D<float4> positionEmissiveTexture = ResourceDescriptorHeap[renderResources.positionEmissiveGBufferIndex];
    Texture2D<float4> normalEmissiveTexture = ResourceDescriptorHeap[renderResources.normalEmissiveGBufferIndex];
    Texture2D<float4> aoMetalRoughnessEmissiveTexture =
        ResourceDescriptorHeap[renderResources.aoMetalRoughnessEmissiveGBufferIndex];

    // const float4 albedo = albedoTexture.Sample(pointClampSampler, psInput.textureCoord);
    float4 albedo = float4(1.0f, 0.0f, 0.0f, 1.0f);

    const float4 positionEmissive = positionEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);
    const float4 normalEmissive = normalEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);
    const float4 aoMetalRoughnessEmissive = aoMetalRoughnessEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);

    const float3 viewSpacePosition = positionEmissive.xyz;
    const float3 normal = normalize(normalEmissive.xyz);

    const float ao = aoMetalRoughnessEmissive.r;
    const float metallicFactor = aoMetalRoughnessEmissive.g;
    const float roughnessFactor = aoMetalRoughnessEmissive.b;

    const float3 emissive = float3(positionEmissive.w, normalEmissive.w, aoMetalRoughnessEmissive.a);

    const float3 viewDirection = normalize(-viewSpacePosition);

    // Reflectance equation for reference.
    // lo(x, v) = le(x, v) + integral(over hemisphere centered at x)(fr(x, l, v, roughness) * li(x, l) * (l.n)dl
    // x is the pixel position
    // v is the view direction
    // l is the light vector
    // lo is the irradiance, while li is the light radiance
    // fr is the brdf term.

    float3 lo = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < interlop::TOTAL_POINT_LIGHTS; ++i)
    {
        const float3 pixelToLightDirection = normalize(lightBuffer.viewSpaceLightPosition[i].xyz - viewSpacePosition);

        const float3 brdf =
            cookTorrenceBRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);

        const float distance = length(lightBuffer.viewSpaceLightPosition[i].xyz - viewSpacePosition);
        const float attenuation = 1.0f / (distance * distance);

        const float3 radiance = lightBuffer.lightColor[i].xyz * lightBuffer.radiusIntensity[i].y;

        lo += brdf * radiance * saturate(dot(pixelToLightDirection, normal)); // *attenuation;
    }

    for (uint j = interlop::DIRECTIONAL_LIGHT_OFFSET;
         j < interlop::DIRECTIONAL_LIGHT_OFFSET + interlop::TOTAL_DIRECTIONAL_LIGHTS; ++j)
    {
        const float3 pixelToLightDirection = normalize(-lightBuffer.viewSpaceLightPosition[j].xyz);

        const float3 brdf =
            cookTorrenceBRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);

        const float3 radiance = lightBuffer.lightColor[j].xyz * lightBuffer.radiusIntensity[j].y;

        float nDotL = saturate(dot(pixelToLightDirection, normal));
        lo += brdf * radiance * nDotL;
    }

    lo += emissive;

    return float4(lo, 1.0f);
}