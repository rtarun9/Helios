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
    ConstantBuffer<PointLightData> pointLightCBuffer = ResourceDescriptorHeap[renderResource.pointLightCBufferIndex];
    ConstantBuffer<DirectionalLightData> directionalLightCBuffer = ResourceDescriptorHeap[renderResource.directionalLightCBufferIndex];
    
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResource.albedoTextureIndex];
    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[renderResource.metalRoughnessTextureIndex];
    
    float3 normal = normalize(input.normal);
    float3 ao = float3(1.0f, 1.0f, 1.0f);
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    
    if (renderResource.normalTextureIndex)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[renderResource.normalTextureIndex];
        
        input.normal = normalize(input.normal);
        normal = normalize(2.0f * normalTexture.Sample(anisotropicSampler, input.texCoord).xyz - float3(1.0f, 1.0f, 1.0f));

        input.tangent.xyz = normalize(input.tangent.xyz);
        
        float3 bitangent = normalize(cross(input.normal, input.tangent.xyz)) * input.tangent.w;
        float3x3 tbn = float3x3(input.tangent.xyz, bitangent, input.normal);
    
        normal = mul(normal, tbn);
        normal = normalize(mul(normal, input.modelMatrix));
    }
    
    if (renderResource.aoTextureIndex)
    {
        Texture2D<float4> aoTexture = ResourceDescriptorHeap[renderResource.aoTextureIndex];
        ao = aoTexture.Sample(linearWrapSampler, input.texCoord).xyz;
    }
    
    if (renderResource.emissiveTextureIndex)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[renderResource.emissiveTextureIndex];
        emissive = emissiveTexture.Sample(linearWrapSampler, input.texCoord).xyz;
    }
    
    float3 viewDir = normalize(cameraCBuffer.cameraPosition.xyz - input.worldSpacePosition.xyz);
    
    float metallicFactor = metalRoughnessTexture.Sample(anisotropicSampler, input.texCoord).b;
    float roughnessFactor = metalRoughnessTexture.Sample(anisotropicSampler, input.texCoord).g;
    float4 albedoColor = albedoTexture.Sample(anisotropicSampler, input.texCoord);
    
    clip(albedoColor.a < 0.1f ? -1.0f : 1.0f);
    float3 albedo = albedoColor.xyz;
    
    float3 lo = float3(0.0f, 0.0f, 0.0f);
    
    // Calculate radiance due to each punctual light source.
    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 lightDirection = normalize(pointLightCBuffer.lightPosition[i].xyz - input.worldSpacePosition.xyz);
        float nDotL = saturate(dot(normal, lightDirection));
        float3 li = pointLightCBuffer.lightColor[i].xyz;
    
        float distance = length(pointLightCBuffer.lightPosition[i].xyz - input.worldSpacePosition.xyz);
        float attenuation = PunctualLightAttenuation(distance, pointLightCBuffer.radius[i]);
        float3 radiance = li * attenuation;
        
        lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo, metallicFactor, roughnessFactor) * radiance * nDotL;
    }
    
    // Calculate irradiance due to each directional light source.
    for (uint k = 0; k < TOTAL_DIRECTIONAL_LIGHTS; ++k)
    {
        float3 lightDirection = normalize(-directionalLightCBuffer.lightDirection[k].xyz);
        float nDotL = saturate(dot(normal, lightDirection));
    
        float3 radiance = directionalLightCBuffer.lightColor[k].xyz;
        
        lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo, metallicFactor, roughnessFactor) * radiance * nDotL;
    }
    
    float3 outgoingLight = lo + emissive;   
    
    if (renderResource.enableIBL)
    {
        // IBL Calculation.
        float3 diffuseIBL = DiffuseIBL(normal, albedo, roughnessFactor, metallicFactor, viewDir, renderResource.irradianceMap);
        float3 specularIBL = SpecularIBL(normal, albedo, viewDir, roughnessFactor, metallicFactor, renderResource.prefilterMap, renderResource.brdfConvolutionLUTMap);
        outgoingLight += (diffuseIBL + specularIBL) * ao;
    }

    
    return float4(outgoingLight, 1.0f);
}   