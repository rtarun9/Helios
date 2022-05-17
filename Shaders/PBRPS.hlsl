#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"
#include "BRDF.hlsli"
#include "IBL.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
    float3x3 modelMatrix : MODEL_MATRIX;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<CameraData> cameraCBuffer = ResourceDescriptorHeap[renderResource.cameraCBufferIndex];
    ConstantBuffer<LightData> lightDataCBuffer = ResourceDescriptorHeap[renderResource.lightDataCBufferIndex];
    
    float3 normal = GetNormal(input.normal, input.texCoord, input.tangent, input.modelMatrix, renderResource.normalTextureIndex);   
    float3 ao = GetAO(input.texCoord, renderResource.aoTextureIndex);
    float3 emissive = GetEmissive(input.texCoord, renderResource.emissiveTextureIndex);
    float4 albedo = GetAlbedo(input.texCoord, renderResource.albedoTextureIndex);
    
    float2 metalRoughnessFactor = GetMetallicRoughnessFactor(input.texCoord, renderResource.metalRoughnessTextureIndex);
    
    float metallicFactor = metalRoughnessFactor.x;
    float roughnessFactor = metalRoughnessFactor.y;
    
    clip(albedo.a < 0.1f ? -1.0f : 1.0f);
  
    float3 viewDir = normalize(cameraCBuffer.cameraPosition.xyz - input.worldSpacePosition.xyz);
    
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
            float3 lightDirection = normalize(lightDataCBuffer.lightPosition[i].xyz - input.worldSpacePosition.xyz);
            float nDotL = saturate(dot(normal, lightDirection));
            float3 li = lightDataCBuffer.lightColor[i].xyz;
    
            float distance = length(lightDataCBuffer.lightPosition[i].xyz - input.worldSpacePosition.xyz);
            float attenuation = PunctualLightAttenuation(distance, lightDataCBuffer.radius[i]);
            float3 radiance = li * attenuation;
        
            lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo.xyz, metallicFactor, roughnessFactor) * radiance * nDotL;
        }
    }
        
    float3 outgoingLight =  float3(0.0f, 0.0f, 0.0f);   
    
    if (renderResource.enableIBL)
    {
        // IBL Calculation.
        float3 diffuseIBL = DiffuseIBL(normal, albedo.xyz, roughnessFactor, metallicFactor, viewDir, renderResource.irradianceMap);
        float3 specularIBL = SpecularIBL(normal, albedo.xyz, viewDir, roughnessFactor, metallicFactor, renderResource.prefilterMap, renderResource.brdfConvolutionLUTMap);
        outgoingLight += (diffuseIBL + specularIBL) * ao + emissive + lo;
    }
    
    return float4(outgoingLight, 1.0f);
}   