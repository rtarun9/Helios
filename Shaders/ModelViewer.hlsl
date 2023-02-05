// clang-format off

#include "Utils.hlsli"
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<interlop::ModelViewerRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID: SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResources.textureCoordBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResources.transformBufferIndex];
    
    VSOutput output;

    output.position = mul(mul(float4(positionBuffer[vertexID].xyz, 1.0f), transformBuffer.modelMatrix), sceneBuffer.viewProjectionMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];

    return output;
}

[RootSignature(BindlessRootSignature)] 
float4 PsMain(VSOutput input) : SV_Target
{
    const float4 albedoColor = getAlbedo(input.textureCoord, renderResources.albedoTextureIndex, renderResources.albedoTextureSamplerIndex);
    return pow(albedoColor, 1/2.2f);    
}