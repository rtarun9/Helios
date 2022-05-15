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
    ConstantBuffer<MaterialData> materialCBuffer = ResourceDescriptorHeap[renderResource.materialCBufferIndex];
    ConstantBuffer<CameraData> cameraCBuffer = ResourceDescriptorHeap[renderResource.cameraCBufferIndex];
    ConstantBuffer<PointLightData> pointLightCBuffer = ResourceDescriptorHeap[renderResource.pointLightCBufferIndex];
    
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResource.albedoTextureIndex];
    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[renderResource.metalRoughnessTextureIndex];

    float3 normal = normalize(input.normal);
    float3 ao = float3(1.0f, 1.0f, 1.0f);
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    
    if (renderResource.normalTextureIndex)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[renderResource.normalTextureIndex];
        
        input.normal = normalize(input.normal);
        normal = normalize(2.0f * normalTexture.Sample(linearWrapSampler, input.texCoord).xyz - float3(1.0f, 1.0f, 1.0f));

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
    
    float metallicFactor = metalRoughnessTexture.Sample(linearWrapSampler, input.texCoord).b;
    float roughnessFactor = metalRoughnessTexture.Sample(linearWrapSampler, input.texCoord).g;
    float3 albedo = albedoTexture.Sample(anisotropicSampler, input.texCoord).xyz;

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    
    // Calculate irradiance due to each light source.
    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 lightDirection = normalize(pointLightCBuffer.lightPosition[i].xyz - input.worldSpacePosition.xyz);
        float nDotL = saturate(dot(normal, lightDirection));
        float3 li = pointLightCBuffer.lightColor[i].xyz;
    
        float distance = length(pointLightCBuffer.lightPosition[i].xyz - input.worldSpacePosition.xyz);
        float attenuation = PointLightAttenuation(distance, pointLightCBuffer.radius[i]);
        float3 radiance = li * attenuation;
        
        Lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo, metallicFactor, roughnessFactor) * radiance * nDotL;
    }
    
    float3 outgoingLight =  + Lo + emissive;
    
    if (renderResource.enableIBL)
    {
        // IBL Calculation.
        float3 diffuseIBL = DiffuseIBL(normal, albedo, roughnessFactor, metallicFactor, viewDir, renderResource.irradianceMap);
        float3 specularIBL = SpecularIBL(normal, albedo, viewDir, roughnessFactor, metallicFactor, renderResource.prefilterMap, renderResource.brdfConvolutionLUTMap);
        outgoingLight += (diffuseIBL + specularIBL) * ao;
    }

    
    return float4(outgoingLight, 1.0f);
}   