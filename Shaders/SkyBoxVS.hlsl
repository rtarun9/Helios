#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 modelSpacePosition : POSITION;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

ConstantBuffer<SkyBoxRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];
  
    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 0.0f), mul(mvpCBuffer.viewMatrix, mvpCBuffer.projectionMatrix));
    output.modelSpacePosition = float4(positionBuffer[vertexID].xyz, 0.0f);
    output.position = output.position.xyww; 
    
    return output;
}