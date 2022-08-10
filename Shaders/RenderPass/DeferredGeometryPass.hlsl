#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float3 worldSpacePosition : WORLD_SPACE_POSITION;
    float3x3 tbnMatrix : TBN_MATRIX;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResource.normalBufferIndex];

    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];
    ConstantBuffer<TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    matrix mvpMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewProjectionMatrix);
    float3x3 normalMatrix = (float3x3)transpose(transformBuffer.inverseModelMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.normal = normalBuffer[vertexID];
    output.worldSpacePosition = mul(float4(positionBuffer[vertexID], 1.0f), transformBuffer.modelMatrix).xyz;
    
    float3 tangent = GenerateTangent(output.normal).xyz;
    float3 biTangent = normalize(cross(output.normal, tangent));

    // Calculation of tbn matrix.
    float3 t = normalize(mul(tangent, normalMatrix));
    float3 b = normalize(mul(biTangent, normalMatrix));
    float3 n = normalize(mul(output.normal, normalMatrix));

    output.tbnMatrix = float3x3(t, b, n);

    return output;
}

struct PsOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 albedo : SV_Target2;
};

[RootSignature(BindlessRootSignature)]
PsOutput PsMain(VSOutput psInput) 
{
    PsOutput output;

    output.position = float4(psInput.worldSpacePosition, 1.0f);
    output.albedo = GetAlbedo(psInput.textureCoord, renderResource.albedoTextureIndex, renderResource.albedoTextureSamplerIndex);

    if (output.albedo.a < 0.9f)
    {
        discard;
    }
    
    output.normal = float4(GetNormal(psInput.textureCoord, renderResource.normalTextureIndex, renderResource.normalTextureSamplerIndex, psInput.normal, psInput.tbnMatrix), 0.0f);

    return output;
}