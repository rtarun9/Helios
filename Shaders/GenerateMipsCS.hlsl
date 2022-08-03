// Primary reference : https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli.

#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

static const uint GROUP_THREAD_COUNT = 8u;

// Main reason for seperation of channels is for reducing bank conflicts (i.e two threads dont access the same memory bank,
// but not necessarily the same address).
groupshared float groupSharedRedChannel[64];
groupshared float groupSharedBlueChannel[64];
groupshared float groupSharedGreenChannel[64];
groupshared float groupSharedAlphaChannel[64];

void StoreColor(uint groupSharedIndex, float4 color)
{
	groupSharedRedChannel[groupSharedIndex] = color.r;
	groupSharedGreenChannel[groupSharedIndex] = color.g;
	groupSharedBlueChannel[groupSharedIndex] = color.b;
	groupSharedAlphaChannel[groupSharedIndex] = color.a;
}

float4 LoadColor(uint groupSharedIndex)
{
	return float4(groupSharedRedChannel[groupSharedIndex], groupSharedGreenChannel[groupSharedIndex], groupSharedBlueChannel[groupSharedIndex], groupSharedAlphaChannel[groupSharedIndex]);
}


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
    RWTexture2D<float4> outputMip1Texture = ResourceDescriptorHeap[renderResources.outputMip1Index];
    RWTexture2D<float4> outputMip2Texture = ResourceDescriptorHeap[renderResources.outputMip2Index];
    RWTexture2D<float4> outputMip3Texture = ResourceDescriptorHeap[renderResources.outputMip3Index];
    RWTexture2D<float4> outputMip4Texture = ResourceDescriptorHeap[renderResources.outputMip4Index];


    uint mipMapGenerationBufferIndex;

    // Color value of current mip level being generated.
    float4 sourceMip1 = float4(0.0f, 0.0f, 0.0f, 1.0f);

    switch (mipMapBuffer.sourceDimensionType)
    {
        case TextureDimensionTypes::WidthHeightEven:
        {
            // This offset is taken so that when we use linear filtering the sampler will sample / blend from four corner pixels of sample location.
            float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + 0.5);

            sourceMip1 = sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel);
        }break;

        case TextureDimensionTypes::WidthHeightOdd:
        {
            // In this case, 4 bilinear samples are chosen to make sure undersampling doesnt occur.
            float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + float2(0.25f, 0.25f));
            float2 offset = mipMapBuffer.texelSize * float2(0.5f, 0.5f);

            sourceMip1 = 0.25f * (sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel) +
                sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + offset, mipMapBuffer.sourceMipLevel) + 
                sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + float2(offset.x ,0.0f), mipMapBuffer.sourceMipLevel) +
                sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + float2(0.0f ,offset.y), mipMapBuffer.sourceMipLevel));

        }break;

        case TextureDimensionTypes::WidthEvenHeightOdd:
        {
            // In this case, 2 bilinear samples are chosen to make sure undersampling doesnt occur.
            float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + float2(0.5f, 0.25f));
            float2 offset = mipMapBuffer.texelSize * float2(0.0f, 0.5f);

            sourceMip1 = 0.5f * (sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel) +
                sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + offset, mipMapBuffer.sourceMipLevel));
        }break;

        case TextureDimensionTypes::WidthOddHeightEven:
        {
            // In this case, 2 bilinear samples are chosen to make sure undersampling doesnt occur.
            float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + float2(0.25f, 0.5f));
            float2 offset = mipMapBuffer.texelSize * float2(0.5f, 0.0f);

            sourceMip1 = 0.5f * (sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel) +
                sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + offset, mipMapBuffer.sourceMipLevel));

        }break;
    }

    outputMip1Texture[dispatchThreadID.xy] = PackColor(sourceMip1, mipMapBuffer.isSRGB);

    if (mipMapBuffer.numberMipLevels == 1)
    {
        return;
    }

    StoreColor(groupIndex, sourceMip1);
    
    GroupMemoryBarrierWithGroupSync();
}