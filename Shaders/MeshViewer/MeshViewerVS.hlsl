#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float3> colorBuffer = ResourceDescriptorHeap[renderResource.colorBufferIndex];

    VSOutput output;
    output.position = float4(positionBuffer[vertexID], 1.0f);
    output.color = float4(colorBuffer[vertexID], 1.0f);
    return output;
}