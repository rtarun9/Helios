// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float3 worldSpaceNormal : WORLD_SPACE_NORMAL;
    float3x3 tbnMatrix : TBN_MATRIX;
    float3x3 viewMatrix : VIEW_MATRIX;
};

ConstantBuffer<interlop::DeferredGPassRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResources.textureCoordBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResources.normalBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResources.transformBufferIndex];

    const matrix mvpMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewProjectionMatrix);
    const matrix mvMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewMatrix);
    const float3x3 normalMatrix = (float3x3)transpose(transformBuffer.inverseModelMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.normal = normalBuffer[vertexID];
    output.worldSpaceNormal = normalize(mul(output.normal, normalMatrix));
    output.viewMatrix = (float3x3)sceneBuffer.viewMatrix;

    const float3 tangent = generateTangent(output.normal).xyz;
    const float3 biTangent = normalize(cross(output.normal, tangent));

    // Calculation of tbn matrix.
    const float3 t = normalize(mul(tangent, normalMatrix));
    const float3 b = normalize(mul(biTangent, normalMatrix));
    const float3 n = normalize(mul(output.normal, normalMatrix));

    output.tbnMatrix = float3x3(t, b, n);

    return output;
}

// Geometry buffer breakdown:
// float4 albedoEmissive : SV_Target0;
// float4 normalEmissive : SV_Target1;
// float4 aoMetalRoughnessEmissive : SV_Target2;
struct PsOutput
{
    float4 albedoEmissive : SV_Target0;
    float4 normalEmissive : SV_Target1;
    float4 aoMetalRoughnessEmissive : SV_Target2;
};

[RootSignature(BindlessRootSignature)] 
PsOutput PsMain(VSOutput psInput) 
{
    ConstantBuffer<interlop::MaterialBuffer> materialBuffer = ResourceDescriptorHeap[renderResources.materialBufferIndex];

    PsOutput output;

    output.albedoEmissive = getAlbedo(psInput.textureCoord, renderResources.albedoTextureIndex, renderResources.albedoTextureSamplerIndex, materialBuffer.albedoColor);
    
    if (output.albedoEmissive.a < 0.9f)
    {
        discard;
    }

    float3 emissive = getEmissive(psInput.textureCoord, output.albedoEmissive.xyz, materialBuffer.emissiveFactor, renderResources.emissiveTextureIndex, renderResources.emissiveTextureSamplerIndex);

    output.albedoEmissive = float4(output.albedoEmissive.xyz, emissive.r);

    output.normalEmissive = float4(getNormal(psInput.textureCoord, renderResources.normalTextureIndex, renderResources.normalTextureSamplerIndex, psInput.normal, psInput.worldSpaceNormal, psInput.tbnMatrix), emissive.g);
    output.normalEmissive.xyz = mul(output.normalEmissive.xyz, psInput.viewMatrix);

    float ao = getAO(psInput.textureCoord, renderResources.aoTextureIndex, renderResources.aoTextureSamplerIndex);
    float2 metalRoughness = getMetalRoughness(psInput.textureCoord, renderResources.metalRoughnessTextureIndex, renderResources.metalRoughnessTextureSamplerIndex) * float2(materialBuffer.metallicFactor, materialBuffer.roughnessFactor);

    output.aoMetalRoughnessEmissive = float4(ao, metalRoughness, emissive.b);

    return output;
}