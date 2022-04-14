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
    matrix projectionViewMatrix;
};

ConstantBuffer<SkyBoxRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];
  
    VSOutput output;
    output.position = mul(mvpCBuffer.projectionViewMatrix, float4(positionBuffer[vertexID], 0.0f));
    output.modelSpacePosition = float4(positionBuffer[vertexID].xyz, 0.0f);
    output.position = output.position.xyww; 
    
    return output;
}