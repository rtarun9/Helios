#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"
#include "Utils.hlsli"

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
    
    output.tangent = float4(0.0f, 0.0f, 0.0f, -1.0f);
    
    if (renderResource.tangetBufferIndex)
    {
        output.tangent = tangetBuffer[vertexID];
    }
    else
    {
        output.tangent = GenerateTangent(output.normal);
    }

    output.worldSpacePosition = mul(float4(positionBuffer[vertexID], 1.0f), mvpCBuffer.modelMatrix);
   
    output.modelMatrix = (float3x3) (mvpCBuffer.modelMatrix);
    return output;
}