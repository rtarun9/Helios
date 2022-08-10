#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];

    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];
    ConstantBuffer<TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    matrix mvpMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewProjectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];

    return output;
}

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    return GetAlbedo(psInput.textureCoord, renderResource.albedoTextureIndex, renderResource.albedoTextureSamplerIndex);    
}