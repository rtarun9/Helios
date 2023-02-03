#include "Common/BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

struct TriangleRenderResources
{
    uint positionBufferIndex;
};
ConstantBuffer<TriangleRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID
                                                       : SV_VertexID) {
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];

    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xyz, 1.0f);

    return output;
}

    [RootSignature(BindlessRootSignature)] float4 PsMain(VSOutput input)
    : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}