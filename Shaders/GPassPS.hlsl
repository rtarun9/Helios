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

struct PSOutput
{
    float4 albedo : SV_Target0;
    float4 position : SV_Target1;
    float4 normal : SV_Target2;
    float4 aoMetalRoughness : SV_Target3;
    float4 emissive : SV_Target4;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
PSOutput PsMain(VSOutput input) 
{
    ConstantBuffer<CameraData> cameraCBuffer = ResourceDescriptorHeap[renderResource.cameraCBufferIndex];
    ConstantBuffer<LightData> lightDataCBuffer = ResourceDescriptorHeap[renderResource.lightDataCBufferIndex];
    
    float3 ao = GetAO(input.texCoord, renderResource.aoTextureIndex);
    
    float2 metalRoughnessFactor = GetMetallicRoughnessFactor(input.texCoord, renderResource.metalRoughnessTextureIndex);
    
    float metallicFactor = metalRoughnessFactor.x;
    float roughnessFactor = metalRoughnessFactor.y;
    
    PSOutput output;
    output.albedo = GetAlbedo(input.texCoord, renderResource.albedoTextureIndex);
    output.position = input.worldSpacePosition;
    output.normal = float4(GetNormal(input.normal, input.texCoord, input.tangent, input.modelMatrix, renderResource.normalTextureIndex), 1.0f);
    output.aoMetalRoughness = float4(ao.x, metallicFactor, roughnessFactor, 1.0f); 
    output.emissive = float4(GetEmissive(input.texCoord, renderResource.emissiveTextureIndex), 1.0f);
    
    return output;
}