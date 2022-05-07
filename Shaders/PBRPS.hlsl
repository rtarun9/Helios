#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"
#include "BRDF.hlsli"

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

static const uint LIGHT_COUNT = 1u;

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<MaterialData> materialCBuffer = ResourceDescriptorHeap[renderResource.materialCBufferIndex];
    ConstantBuffer<LightingData> lightCBuffer = ResourceDescriptorHeap[renderResource.lightCBufferIndex];
   
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResource.albedoTextureIndex];
    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[renderResource.metalRoughnessTextureIndex];

    float3 normal = normalize(input.normal);
    float3 ao = float3(1.0f, 1.0f, 1.0f);
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    
    if (renderResource.normalTextureIndex)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[renderResource.normalTextureIndex];
        
        input.normal = normalize(input.normal);
        normal = normalize(2.0f * normalTexture.Sample(pointWrapSampler, input.texCoord).xyz - float3(1.0f, 1.0f, 1.0f));

        if (input.tangent.w != -1.0f)
        {
            input.tangent.xyz = normalize(input.tangent.xyz);
        
            float3 bitangent = normalize(cross(input.normal, input.tangent.xyz)) * input.tangent.w;
            float3x3 tbn = float3x3(input.tangent.xyz, bitangent, input.normal);
    
            normal = mul(normal, tbn);
            normal = normalize(mul(normal, input.modelMatrix));
        }
    }
    
    if (renderResource.aoTextureIndex)
    {
        Texture2D<float4> aoTexture = ResourceDescriptorHeap[renderResource.aoTextureIndex];
        ao = aoTexture.Sample(pointWrapSampler, input.texCoord).xyz;
    }
    
    if (renderResource.emissiveTextureIndex)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[renderResource.emissiveTextureIndex];
        emissive = emissiveTexture.Sample(pointWrapSampler, input.texCoord).xyz;
    }
    
    float3 viewDir = normalize(lightCBuffer.cameraPosition.xyz - input.worldSpacePosition.xyz);
    
    float metallicFactor = metalRoughnessTexture.Sample(pointWrapSampler, input.texCoord).b;
    float roughnessFactor = metalRoughnessTexture.Sample(pointWrapSampler, input.texCoord).g;
    float3 albedo = albedoTexture.Sample(pointWrapSampler, input.texCoord).xyz;

    //albedo = materialCBuffer.albedo;
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallicFactor);
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    float3 lightDirection = normalize(lightCBuffer.lightPosition.xyz - input.worldSpacePosition.xyz);
    float nDotL = saturate(dot(normal, lightDirection));
    float3 li = float3(1.0f, 1.0f, 1.0f);
    
    float distance = length(lightDirection);
    float attenuation = 1.0 / max((pow(distance, 2)), MIN_FLOAT_VALUE);
    float3 radiance = li * attenuation;
    
    // Calculate irradiance due to each light source.
    for (uint i = 0; i < LIGHT_COUNT; ++i)
    {
        Lo += CookTorrenceBRDF(normal, viewDir, lightDirection, albedo, metallicFactor, roughnessFactor) * radiance * nDotL;
    }
    
    // IBL Calculation.
    
    // For diffuse IBL.
    TextureCube<float4> irradianceMap = ResourceDescriptorHeap[renderResource.irradianceMap];
    float3 irradiance = irradianceMap.SampleLevel(pointWrapSampler, normal.xyz, 0.0f).xyz;

    float cosLO = max(dot(normal, viewDir), 0.0f);
    float3 LR = reflect(-viewDir, normal);

    float3 kS = FresnelSchlick(cosLO, F0, roughnessFactor);
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), metallicFactor);

    float3 F = kS;
    
    float3 diffuseIBL = irradiance * albedo;
    
    // For Specular IBL.
    TextureCube<float4> specularIrradianceMap = ResourceDescriptorHeap[renderResource.prefilterMap];
    Texture2D<float2> specularBRDF = ResourceDescriptorHeap[renderResource.brdfConvolutionLUTMap];
    
    // Get number of mip levels.
    uint specularTextureWidth, specularTextureHeight, levels;
    specularIrradianceMap.GetDimensions(0u, specularTextureWidth, specularTextureHeight, levels);

    float3 specularIrradiance = specularIrradianceMap.SampleLevel(pointWrapSampler, LR, roughnessFactor * 5.0f).rgb;
    
    float2 specularBRDFLUT = specularBRDF.Sample(pointWrapSampler, float2(cosLO, roughnessFactor), 0.0f).rg;

    float3 specularIBL = specularIrradiance * (F0 * specularBRDFLUT.x + specularBRDFLUT.y);
    
    float3 outgoingLight = (specularIBL + diffuseIBL * kD) * ao + Lo + emissive * 3.0f;
    
    return float4(outgoingLight, 1.0f);
}   