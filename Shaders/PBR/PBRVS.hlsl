#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResource.normalBufferIndex];

    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];
    ConstantBuffer<TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    matrix mvpMatrix = mul(mul(transformBuffer.modelMatrix, sceneBuffer.viewMatrix), sceneBuffer.projectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.normal = normalBuffer[vertexID];

    return output;
}