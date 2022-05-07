#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 modelSpacePosition : POSITION;
};

ConstantBuffer<SkyBoxRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];
  
    matrix vpMatrix = mul(mvpCBuffer.viewMatrix, mvpCBuffer.projectionMatrix);
    
    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 0.0f), vpMatrix);
    output.modelSpacePosition = float4(positionBuffer[vertexID].xyz, 0.0f);
    output.position = output.position.xyww; 
    
    return output;
}