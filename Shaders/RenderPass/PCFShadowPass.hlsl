// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

ConstantBuffer<interlop::ShadowPassRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    
    ConstantBuffer<interlop::TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResources.transformBufferIndex];
    ConstantBuffer<interlop::ShadowBuffer> shadowBuffer = ResourceDescriptorHeap[renderResources.shadowBufferIndex];

    const matrix mvpMatrix = mul(transformBuffer.modelMatrix, shadowBuffer.lightViewProjectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    return output;
}

[RootSignature(BindlessRootSignature)] 
void PsMain(VSOutput input) 
{
}