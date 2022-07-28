#include "../Common/BindlessRS.hlsli"

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    Texture2D albedoTexture = ResourceDescriptorHeap[renderResource.albedoTextureIndex];
    return albedoTexture.Sample(linearWrapSampler, psInput.textureCoord);
}