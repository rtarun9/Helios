// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::BoxBlurRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(12, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    static const int BLUR_KERNEL_DIMENSIONS = 9u;
    Texture2D<float> tex = ResourceDescriptorHeap[renderResources.textureIndex];
    float2 texDimensions;
    tex.GetDimensions(texDimensions.x, texDimensions.y);

    RWTexture2D<float> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    const float2 pixelSize = 1.0f / texDimensions;
    const float2 uv = (dispatchThreadID.xy + 0.5f) * pixelSize;

    float sum = 0.0f;
    [unroll(9)]
    for (int k = 0; k < BLUR_KERNEL_DIMENSIONS; ++k)
    {
        float x = k / 3.0f;
        float y = float(k % 3);

        const float2 offset = float2(x, y) * pixelSize;
        sum += tex.Sample(linearClampSampler, uv + offset).r;
    }
    
    outputTexture[dispatchThreadID.xy] = sum / 9.0f;
}