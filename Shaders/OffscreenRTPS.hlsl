#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"

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
    Texture2D<float4> deferredPassRTVTexture = ResourceDescriptorHeap[renderResource.deferredPassTextureIndex];
    
    ConstantBuffer<RenderTargetSettings> settings= ResourceDescriptorHeap[renderResource.renderTargetSettingsCBufferIndex];
    
    // Tone mapping logic will eventually be made better by using: https://www.cis.rit.edu/people/faculty/ferwerda/publications/sig02_paper.pdf
    float3 hdrColor = (rtvTexture.Sample(pointClampSampler, input.textureCoord).xyz + deferredPassRTVTexture.Sample(pointClampSampler, input.textureCoord).xyz) * settings.exposure;
    
    float3 toneMappedColor = float3(1.0f, 1.0f, 1.0f) - exp(-hdrColor * settings.exposure);
    toneMappedColor = pow(toneMappedColor, GAMMA_CORRECTION);
    
    return float4(toneMappedColor, 1.0f);
}