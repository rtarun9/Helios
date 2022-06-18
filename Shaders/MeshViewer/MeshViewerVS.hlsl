#include "../BindlessRS.hlsli"
#include "../ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];


    VSOutput output;
    output.position = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}