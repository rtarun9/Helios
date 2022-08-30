#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"

ConstantBuffer<BloomPassRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> inputTexture = ResourceDescriptorHeap[renderResources.inputTextureIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    outputTexture[dispatchThreadID.xy] = inputTexture.SampleLevel(linearClampSampler, dispatchThreadID.xy, 0u);
}