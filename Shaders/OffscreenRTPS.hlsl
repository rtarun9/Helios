#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<RenderTargetRenderResources> renderResource : register(b0);

static const float GAMMA_CORRECTION = 0.454545455f;

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    Texture2D<float4> rtvTexture = ResourceDescriptorHeap[renderResource.textureIndex];
    ConstantBuffer<PostProcessBuffer> postProcessBuffer = ResourceDescriptorHeap[renderResource.postProcessBufferIndex];

    float exposure = postProcessBuffer.exposure;

    float4 color = rtvTexture.Sample(pointWrapSampler, input.textureCoord);

    // Exposure Tone mapping
    color.rgb = float3(1.0f, 1.0f, 1.0f) - exp(-color.rgb * exposure);
    
    // Gamma correction.
    color.rgb = pow(color.rgb, GAMMA_CORRECTION);

    return color;
}