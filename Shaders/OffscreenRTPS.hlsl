#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<RenderTargetRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);

    Texture2D<float4> rtvTexture = ResourceDescriptorHeap[renderResource.textureIndex];

    return rtvTexture.Sample(pointWrapSampler, input.textureCoord);
}