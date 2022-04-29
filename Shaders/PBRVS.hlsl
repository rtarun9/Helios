#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
    float3x3 TBN : TBN_MATRIX;
    float3x3 modelMatrix : MODEL_MATRIX;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix projectionViewMatrix;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordsBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResource.normalBufferIndex];
    StructuredBuffer<float3> biTangetBuffer = ResourceDescriptorHeap[renderResource.biTangetBufferIndex];
    StructuredBuffer<float4> tangetBuffer = ResourceDescriptorHeap[renderResource.tangetBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];

    matrix mvpMatrix = mul(mvpCBuffer.projectionViewMatrix, mvpCBuffer.modelMatrix);

    
    VSOutput output;
    output.position = mul(mvpMatrix, float4(positionBuffer[vertexID], 1.0f));
    output.texCoord = textureCoordsBuffer[vertexID];
    output.normal = normalBuffer[vertexID];
    output.tangent = tangetBuffer[vertexID];
    output.worldSpacePosition = mul(mvpCBuffer.modelMatrix, float4(positionBuffer[vertexID], 1.0f));

    float3 normal = normalize(normalBuffer[vertexID].xyz);
    float3 tangent = normalize(tangetBuffer[vertexID].xyz);
    float3 biTangent = normalize(biTangetBuffer[vertexID].xyz);
    
    float3 T = normalize(tangent);
    float3 B = normalize(biTangent);
    float3 N = normalize(normal);
    
    output.TBN = float3x3(N, B, T);
   
    output.modelMatrix = (float3x3) mvpCBuffer.modelMatrix;
    return output;
}