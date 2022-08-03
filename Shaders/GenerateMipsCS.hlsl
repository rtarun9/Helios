// Primary reference : https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli.

// NOTE : The logic is heavily simplified and will need to be modified and made more accurate in the future.
#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

static const uint GROUP_THREAD_COUNT = 8u;

// Source: https://en.wikipedia.org/wiki/SRGB#The_reverse_transformation
float3 ConvertToLinear(float3 x)
{
	return x < 0.04045f ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

// Source: https://en.wikipedia.org/wiki/SRGB#The_forward_transformation_(CIE_XYZ_to_sRGB)
float3 ConvertToSRGB(float3 x)
{
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
}

// Convert linear color to sRGB before storing if the original source is 
// an sRGB texture.
float4 PackColor(float4 color, bool isSRGB)
{
    if (isSRGB)
    {
        return float4(ConvertToSRGB(color.rgb), color.a);
    }
    else
    {
        return color;
    }
}

ConstantBuffer<MipMapGenerationRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    ConstantBuffer<MipMapGenerationBuffer> mipMapBuffer = ResourceDescriptorHeap[renderResources.mipMapGenerationBufferIndex];

    Texture2D<float4> sourceMipTexture = ResourceDescriptorHeap[renderResources.sourceMipIndex];
    RWTexture2D<float4> outputMipTexture = ResourceDescriptorHeap[renderResources.outputMipIndex];

    // Color value of current mip level being generated.
    float4 source = float4(0.0f, 0.0f, 0.0f, 1.0f);

    // This offset is taken so that when we use linear filtering the sampler will sample / blend from four corner pixels of sample location.
    float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + 0.5);

    source = sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, renderResources.sourceMipIndex);

    outputMipTexture[dispatchThreadID.xy] = PackColor(source, mipMapBuffer.isSRGB);
}