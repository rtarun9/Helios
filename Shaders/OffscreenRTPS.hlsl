#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<RenderTargetRenderResources> renderResource : register(b0);

SamplerState pointSampler : register(s0, space1);

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    Texture2D<float4> rtvTexture = ResourceDescriptorHeap[renderResource.textureIndex];
    
    return rtvTexture.Sample(pointSampler, input.textureCoord);
}