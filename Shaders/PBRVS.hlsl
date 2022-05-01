#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 worldSpacePosition : WORLD_SPACE_POSITION;
    float3x3 modelMatrix : MODEL_MATRIX;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordsBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResource.normalBufferIndex];
    StructuredBuffer<float4> tangetBuffer = ResourceDescriptorHeap[renderResource.tangetBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];

    matrix mvpMatrix = mul(mul(mvpCBuffer.modelMatrix, mvpCBuffer.viewMatrix), mvpCBuffer.projectionMatrix);

    
    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.texCoord = textureCoordsBuffer[vertexID];
    output.normal = normalBuffer[vertexID];
    output.tangent = tangetBuffer[vertexID];
    output.worldSpacePosition = mul(positionBuffer[vertexID], (float3x3) mvpCBuffer.modelMatrix);
   
    output.modelMatrix = (float3x3) (mvpCBuffer.modelMatrix);
    return output;
}