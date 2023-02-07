// Primary reference :
// https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli.

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"


static const uint GROUP_THREAD_COUNT = 8u;

// Main reason for seperation of channels is for reducing bank conflicts (i.e two threads dont access the same memory
// bank, but not necessarily the same address).
groupshared float groupSharedRedChannel[64];
groupshared float groupSharedGreenChannel[64];
groupshared float groupSharedBlueChannel[64];
groupshared float groupSharedAlphaChannel[64];

void storeColor(uint groupSharedIndex, float4 color)
{
    groupSharedRedChannel[groupSharedIndex] = color.r;
    groupSharedGreenChannel[groupSharedIndex] = color.g;
    groupSharedBlueChannel[groupSharedIndex] = color.b;
    groupSharedAlphaChannel[groupSharedIndex] = color.a;
}

float4 loadColor(uint groupSharedIndex)
{
    return float4(groupSharedRedChannel[groupSharedIndex], groupSharedGreenChannel[groupSharedIndex],
                  groupSharedBlueChannel[groupSharedIndex], groupSharedAlphaChannel[groupSharedIndex]);
}

// Source: https://en.wikipedia.org/wiki/SRGB#The_reverse_transformation
float3 convertToLinear(float3 x)
{
    return x < 0.04045f ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

// Source: https://en.wikipedia.org/wiki/SRGB#The_forward_transformation_(CIE_XYZ_to_sRGB)
float3 convertToSRGB(float3 x)
{
    return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
}

// Convert linear color to sRGB before storing if the original source is
// an sRGB texture.
float4 packColor(float4 color, bool isSRGB)
{
    if (isSRGB)
    {
        return float4(convertToSRGB(color.rgb), color.a);
    }
    else
    {
        return color;
    }
}

ConstantBuffer<interlop::MipMapGenerationRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)][numthreads(8, 8, 1)] void CsMain(uint3 dispatchThreadID
                                                                        : SV_DispatchThreadID, uint groupIndex
                                                                        : SV_GroupIndex) {
    ConstantBuffer<interlop::MipMapGenerationBuffer> mipMapBuffer =
        ResourceDescriptorHeap[renderResources.mipMapGenerationBufferIndex];

    Texture2D<float4> sourceMipTexture = ResourceDescriptorHeap[renderResources.sourceMipIndex];
    RWTexture2D<float4> outputMip1Texture = ResourceDescriptorHeap[renderResources.outputMip1Index];
    RWTexture2D<float4> outputMip2Texture = ResourceDescriptorHeap[renderResources.outputMip2Index];
    RWTexture2D<float4> outputMip3Texture = ResourceDescriptorHeap[renderResources.outputMip3Index];
    RWTexture2D<float4> outputMip4Texture = ResourceDescriptorHeap[renderResources.outputMip4Index];

    // Color value of current mip level being generated.
    float4 sourceMip1 = float4(0.0f, 0.0f, 0.0f, 0.0f);

    switch (mipMapBuffer.dimensionType)
    {
    case interlop::TextureDimensionType::HeightWidthEven: {
        // This offset is taken so that when we use linear filtering the sampler will sample / blend from four corner
        // pixels of sample location.(i.e bilinear sampling)
        float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + 0.5);

        sourceMip1 = sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel);
    }
    break;

    case interlop::TextureDimensionType::HeightWidthOdd: {
        // In this case, 4 bilinear samples are chosen to make sure undersampling doesnt occur.
        float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + float2(0.25f, 0.25f));
        float2 offset = mipMapBuffer.texelSize * float2(0.5f, 0.5f);

        sourceMip1 =
            0.25f * (sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel) +
                     sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + offset, mipMapBuffer.sourceMipLevel) +
                     sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + float2(offset.x, 0.0f),
                                                  mipMapBuffer.sourceMipLevel) +
                     sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + float2(0.0f, offset.y),
                                                  mipMapBuffer.sourceMipLevel));
    }
    break;

    case interlop::TextureDimensionType::HeightOddWidthEven: {
        // In this case, 2 bilinear samples are chosen to make sure undersampling doesnt occur.
        float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + float2(0.5f, 0.25f));
        float2 offset = mipMapBuffer.texelSize * float2(0.0f, 0.5f);

        sourceMip1 =
            0.5f * (sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel) +
                    sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + offset, mipMapBuffer.sourceMipLevel));
    }
    break;

    case interlop::TextureDimensionType::HeightEvenWidthOdd: {
        // In this case, 2 bilinear samples are chosen to make sure undersampling doesnt occur.
        float2 uvCoords = mipMapBuffer.texelSize * (dispatchThreadID.xy + float2(0.25f, 0.5f));
        float2 offset = mipMapBuffer.texelSize * float2(0.5f, 0.0f);

        sourceMip1 =
            0.5f * (sourceMipTexture.SampleLevel(linearClampSampler, uvCoords, mipMapBuffer.sourceMipLevel) +
                    sourceMipTexture.SampleLevel(linearClampSampler, uvCoords + offset, mipMapBuffer.sourceMipLevel));
    }
    break;
    }

    outputMip1Texture[dispatchThreadID.xy] = packColor(sourceMip1, mipMapBuffer.isSRGB);

    if (mipMapBuffer.numberMipLevels == 1)
    {
        return;
    }

    storeColor(groupIndex, sourceMip1);

    GroupMemoryBarrierWithGroupSync();

    // For 2nd mip level, only 1/4th of threads are required to participate (1/2 for width, 1/2 for height).
    // 0x9 is b'001001, and if 0x9 & groupIndex == 0, groupIndex is even.
    if ((groupIndex & 0x9) == 0)
    {
        float4 sourceMip2 = loadColor(groupIndex + 0x01); // color to right of current thread.
        float4 sourceMip3 = loadColor(groupIndex + 0x08); // color below current thread
        float4 sourceMip4 = loadColor(
            groupIndex + 0x09); // color right - below current thread (check reference linked above for explanation).
        sourceMip1 = 0.25 * (sourceMip1 + sourceMip2 + sourceMip3 + sourceMip4);

        outputMip2Texture[dispatchThreadID.xy / 2] = packColor(sourceMip1, mipMapBuffer.isSRGB);
        storeColor(groupIndex, sourceMip1);
    }

    if (mipMapBuffer.numberMipLevels == 2)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // For 3rd mip level, only 1/16th of threads are required to participate (1/4 for width, 1/4 for height).
    // 0x1B is b'011011, and if 0x1b & groupIndex == 0, groupIndex is multiple of 4.
    if ((groupIndex & 0x1B) == 0)
    {
        float4 sourceMip2 = loadColor(groupIndex + 0x02);
        float4 sourceMip3 = loadColor(groupIndex + 0x10);
        float4 sourceMip4 = loadColor(groupIndex + 0x12);
        sourceMip1 = 0.25 * (sourceMip1 + sourceMip2 + sourceMip3 + sourceMip4);

        outputMip3Texture[dispatchThreadID.xy / 4] = packColor(sourceMip1, mipMapBuffer.isSRGB);
        storeColor(groupIndex, sourceMip1);
    }

    if (mipMapBuffer.numberMipLevels == 3)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // For 4th mip level, only 1/32th of threads are required to participate (1/16 for width, 1/16 for height).
    // Only one thread will fit this criteria (thrad index 0). Also, the thread group is of size 8 x 8 x 1, and only
    // thread 0 wil satisfy below condition.
    if (groupIndex == 0)
    {
        float4 sourceMip2 = loadColor(groupIndex + 0x04);
        float4 sourceMip3 = loadColor(groupIndex + 0x20);
        float4 sourceMip4 = loadColor(groupIndex + 0x24);
        sourceMip1 = 0.25 * (sourceMip1 + sourceMip2 + sourceMip3 + sourceMip4);

        outputMip4Texture[dispatchThreadID.xy / 8] = packColor(sourceMip1, mipMapBuffer.isSRGB);
    }
}