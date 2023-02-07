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
    output.color = lightBuffer.lightColor[instanceID] * lightBuffer.radiusIntensity[instanceID][1];
    return output;
}

    [RootSignature(BindlessRootSignature)] float4 PsMain(VSOutput psInput)
    : SV_Target
{
    return psInput.color;
}