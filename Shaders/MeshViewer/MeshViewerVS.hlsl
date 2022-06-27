#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

struct CameraDataTest
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float3> colorBuffer = ResourceDescriptorHeap[renderResource.colorBufferIndex];

    ConstantBuffer<CameraDataTest> cameraBuffer = ResourceDescriptorHeap[renderResource.cameraDataBufferIndex];

    matrix mvpMatrix = mul(mul(cameraBuffer.modelMatrix, cameraBuffer.viewMatrix), cameraBuffer.projectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.color = float4(colorBuffer[vertexID], 1.0f);
    return output;
}