// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

ConstantBuffer<interlop::LightRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID
                                                       : SV_VertexID, uint instanceID
                                                       : SV_InstanceID) {
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];

    ConstantBuffer<interlop::LightInstancedRenderingBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    const matrix mvpMatrix = mul(transformBuffer.modelMatrix[instanceID], sceneBuffer.viewProjectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);

    // Adding one since the directional light (which does not run this shader) is at index 0, and instancenID starts from 0.
    output.color = lightBuffer.lightColor[instanceID + 1u] * lightBuffer.radiusIntensity[instanceID + 1u][1];
    return output;
}

    [RootSignature(BindlessRootSignature)] float4 PsMain(VSOutput psInput)
    : SV_Target
{
    return psInput.color;
}