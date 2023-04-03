// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::BoxBlurRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 4, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> tex = ResourceDescriptorHeap[renderResources.textureIndex];
    uint width, height;
    tex.GetDimensions(width, height);

    RWTexture2D<float> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    const float2 pixelSize = float2(1.0f / width, 1.0f / height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * pixelSize;

    float sum = 0.0f;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            const float2 offset = float2(float(x), float(y)) * pixelSize;
            sum += tex.Sample(linearClampSampler, uv + offset).r;
        }
    }
    
    outputTexture[dispatchThreadID.xy] = sum / 9.0f;
}