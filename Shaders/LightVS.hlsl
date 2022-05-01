#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

ConstantBuffer<LightRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];
    
    matrix mvpMatrix = mul(mul(mvpCBuffer.modelMatrix, mvpCBuffer.viewMatrix), mvpCBuffer.projectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);

    return output;
}