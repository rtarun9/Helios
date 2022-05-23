#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

ConstantBuffer<ShadowPassRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[renderResource.mvpCBufferIndex];
    ConstantBuffer<ShadowMappingData> shadowMVPCBuffer = ResourceDescriptorHeap[renderResource.shadowMappingCBufferIndex];

    matrix mvpMatrix = mul(mul(mvpCBuffer.modelMatrix, shadowMVPCBuffer.lightViewMatrix), shadowMVPCBuffer.lightProjectionMatrix);

    
    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);

    return output;
}