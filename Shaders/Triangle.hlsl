#include "Common/BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

struct TriangleRenderResources
{
    uint positionBufferIndex;
    uint textureCoordBufferIndex;
    uint textureIndex;
};
ConstantBuffer<TriangleRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID
                                                       : SV_VertexID) {
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResource.textureCoordBufferIndex];

    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xyz, 1.0f);
    output.textureCoord = textureCoordBuffer[vertexID];

    return output;
}

    [RootSignature(BindlessRootSignature)] float4 PsMain(VSOutput input)
    : SV_Target
{
    Texture2D<float4> testTexture = ResourceDescriptorHeap[renderResource.textureIndex];
    return testTexture.Sample(pointClampSampler, input.textureCoord);
}