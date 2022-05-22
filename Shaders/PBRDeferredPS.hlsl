#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"
#include "BRDF.hlsli"
#include "IBL.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<DeferredPassRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<CameraData> cameraCBuffer = ResourceDescriptorHeap[renderResource.cameraCBufferIndex];
    ConstantBuffer<LightData> lightDataCBuffer = ResourceDescriptorHeap[renderResource.lightDataCBufferIndex];
    
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResource.albedoGPassSRVIndex];
    float4 albedo = albedoTexture.Sample(linearWrapSampler, input.textureCoord);
    
    if (albedo.a < 0.1f)
    {
        discard;
    }
    
    Texture2D<float4> positionTexture = ResourceDescriptorHeap[renderResource.positionGPassSRVIndex];
    float4 worldSpacePosition = positionTexture.Sample(linearWrapSampler, input.textureCoord);
    
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[renderResource.normalGPassSRVIndex];
    float3 normal = normalTexture.Sample(anisotropicSampler, input.textureCoord).xyz;
    
    Texture2D<float4> aoMetalRoughnessTexture = ResourceDescriptorHeap[renderResource.aoMetalRoughnessGPassSRVIndex];
    float3 aoMetalRoughness = aoMetalRoughnessTexture.Sample(anisotropicSampler, input.textureCoord).xyz;
    
    float3 ao = aoMetalRoughness.xxx;
    float metallicFactor = aoMetalRoughness.y;
    float roughnessFactor = aoMetalRoughness.z;
    
    Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[renderResource.emissiveGPassSRVIndex];
    float3 emissive = emissiveTexture.Sample(anisotropicSampler, input.textureCoord).xyz;
  
    clip(albedo.a < 0.1f ? -1.0f : 1.0f);

    float3 viewDir = normalize(cameraCBuffer.cameraPosition.xyz - worldSpacePosition.xyz);
    
    float3 lo = float3(0.0f, 0.0f, 0.0f);
    // Calculate radiance due to each light source
    for (uint i = 0; i < TOTAL_LIGHTS; ++i)
    {
        // if directional light.
        if (lightDataCBuffer.lightPosition[i].w == 0.0f)
        {
            float3 lightDirection = normalize(-lightDataCBuffer.lightPosition[i].xyz);
            float nDotL = saturate(dot(normal, lightDirection));
    
            float3 radiance = lightDataCBuffer.lightColor[i].xyz;
        
            lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo.xyz, metallicFactor, roughnessFactor) * radiance * nDotL;
        }
        // if positional light
        else if (lightDataCBuffer.lightPosition[i].w == 1.0f)
        {
            float3 lightDirection = normalize(lightDataCBuffer.lightPosition[i].xyz - worldSpacePosition.xyz);
            float nDotL = saturate(dot(normal, lightDirection));
            float3 li = lightDataCBuffer.lightColor[i].xyz;
    
            float distance = length(lightDataCBuffer.lightPosition[i].xyz - worldSpacePosition.xyz);
            float attenuation = PunctualLightAttenuation(distance, lightDataCBuffer.radius[i]);
            float3 radiance = li * attenuation;
        
            lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo.xyz, metallicFactor, roughnessFactor) * radiance * nDotL;
        }
    }
        
    float3 outgoingLight = lo + emissive;
    
    if (renderResource.enableIBL)
    {
        // IBL Calculation.
        float3 diffuseIBL = DiffuseIBL(normal, albedo.xyz, roughnessFactor, metallicFactor, viewDir, renderResource.irradianceMap);
        float3 specularIBL = SpecularIBL(normal, albedo.xyz, viewDir, roughnessFactor, metallicFactor, renderResource.prefilterMap, renderResource.brdfConvolutionLUTMap);
        outgoingLight += (diffuseIBL + specularIBL) * ao;
    }
    
    return float4(outgoingLight, 1.0f);
}