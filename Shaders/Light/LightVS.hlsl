#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

ConstantBuffer<LightRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];

    ConstantBuffer<InstanceLightBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    matrix mvpMatrix = mul(mul(transformBuffer.modelMatrix[instanceID], sceneBuffer.viewMatrix), sceneBuffer.projectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.color = lightBuffer.lightColor[instanceID];
    return output;
}