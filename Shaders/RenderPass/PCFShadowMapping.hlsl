#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

ConstantBuffer<ShadowMappingRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];

    ConstantBuffer<TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];
    ConstantBuffer<ShadowMappingBuffer> shadowMappingBuffer = ResourceDescriptorHeap[renderResource.shadowMappingBufferIndex];

    matrix mvpMatrix = mul(transformBuffer.modelMatrix, shadowMappingBuffer.viewProjectionMatrix);
    float3x3 normalMatrix = (float3x3)transpose(transformBuffer.inverseModelMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    return output;
}

[RootSignature(BindlessRootSignature)]
void PsMain(VSOutput psInput) 
{
}