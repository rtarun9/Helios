// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<interlop::TriangleRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID: SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResources.textureCoordBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    
    VSOutput output;

    output.position = mul(float4(positionBuffer[vertexID].xyz, 1.0f), sceneBuffer.viewProjectionMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];

    return output;
}

[RootSignature(BindlessRootSignature)] 
float4 PsMain(VSOutput input) : SV_Target
{
    Texture2D<float4> testTexture = ResourceDescriptorHeap[renderResources.textureIndex];
    return testTexture.Sample(pointClampSampler, input.textureCoord);
}