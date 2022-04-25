#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 modelSpacePosition : POSITION;
};

ConstantBuffer<SkyBoxRenderResources> renderResource : register(b0);

float4 PsMain(VSOutput input) : SV_Target
{
    TextureCube environmentTexture = ResourceDescriptorHeap[renderResource.textureIndex];
 
    float3 samplingVector = normalize(input.modelSpacePosition.xyz);
    samplingVector.y *= -1.0f;

    return environmentTexture.SampleLevel(linearWrapSampler, samplingVector, 0.0f);
}