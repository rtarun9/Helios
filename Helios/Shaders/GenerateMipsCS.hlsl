static const uint THREAD_GROUP_SIZE = 8;

// Various possible conditions based on texture dimension.
static const uint WIDTH_HEIGHT_EVEN = 0;
static const uint WIDTH_EVEN_HEIGHT_ODD = 1;
static const uint WIDTH_ODD_HEIGHT_EVEN = 2;
static const uint WIDTH_HEIGHT_ODD = 3;

struct CSInput
{
    uint3 groupID : SV_GroupID;
    uint3 groupThreadID : SV_GroupThreadID;
    uint3 dispatchThreadID : SV_DispatchThreadID;
    uint groupIndex : SV_GroupIndex;
};

cbuffer GenerateMipCBuffer : register(b0)
{
    float sourceMipLevel;
    uint numberOutputMipLevels;
    uint sourceMipDimensionType;
    bool isSRGB;
    float2 texelSize;
};

Texture2D<float4> sourceMip : register(t0);

RWTexture2D<float4> outputMip1 : register(u0);
RWTexture2D<float4> outputMip2 : register(u1);
RWTexture2D<float4> outputMip3 : register(u2);
RWTexture2D<float4> outputMip4 : register(u3);

SamplerState linearClampSampler : register(s0);

#define GenerateMips_RootSignature                          \
    "RootFlags(0), "                                        \
    "RootConstants(b0, num32BitConstants = 6), "            \
    "DescriptorTable( SRV(t0, numDescriptors = 1) ),"       \
    "DescriptorTable( UAV(u0, numDescriptors = 4) ),"       \
    "StaticSampler(s0,"                                     \
        "addressU = TEXTURE_ADDRESS_CLAMP,"                 \
        "addressV = TEXTURE_ADDRESS_CLAMP,"                 \
        "addressW = TEXTURE_ADDRESS_CLAMP,"                 \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

// When downsampling more than once, samples from a different thread in a group can be shared so sampling same texel
// from global texture memory is not needed.
groupshared float gsRed[64];
groupshared float gsBlue[64];
groupshared float gsGreen[64];
groupshared float gsAlpha[64];

void StoreColor(uint index, float4 color)
{
    gsRed[index] = color.r;
    gsGreen[index] = color.g;
    gsBlue[index] = color.b;
    gsAlpha[index] = color.a;
}

float4 LoadColor(uint index)
{
    return float4(gsRed[index], gsGreen[index], gsBlue[index], gsAlpha[index]);
}

// Note : SRGB <--> Linear formulas from Wikipedia (// Source: https://en.wikipedia.org/wiki/SRGB#The_reverse_transformation)
float3 SRGBToLinear(float3 srgbColor)
{
    return srgbColor < 0.04045f ? srgbColor / 12.92 : pow((srgbColor + 0.055) / 1.055, 2.4);
}

float3 LinearToSRGB(float3 linearColor)
{
    return linearColor < 0.0031308 ? 12.92 * linearColor : 1.055 * pow(abs(linearColor), 1.0 / 2.4) - 0.055;
}

float4 PackColor(float4 color)
{
    if (isSRGB)
    {
        return float4(LinearToSRGB(color.rgb), color.a);
    }
    
    return color;
}

[RootSignature(GenerateMips_RootSignature)]
[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void CsMain(CSInput input)
{
    float4 sourceMipColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    switch (sourceMipDimensionType)
    {
        case WIDTH_HEIGHT_EVEN:
        {
            // Single sample is taken at offset of single pixel, so the sampler will blend the four corner pixels evenly.
            float2 uv = texelSize * (input.dispatchThreadID.xy + 0.5f);
            sourceMipColor = sourceMip.SampleLevel(linearClampSampler, uv, sourceMipLevel);
            break;
        }

        case WIDTH_ODD_HEIGHT_EVEN:
        {
            // Multiple samples taken horizontally.
            float2 uv = texelSize * (input.dispatchThreadID.xy + float2(0.2f, 0.5f));
            float2 offset = texelSize * float2(0.5f, 0.0f);

            sourceMipColor = 0.5f * (sourceMip.SampleLevel(linearClampSampler, uv, sourceMipLevel) + sourceMip.SampleLevel(linearClampSampler, uv + offset, sourceMipLevel));
            break;
        }

        case WIDTH_EVEN_HEIGHT_ODD:
        {
            // Multiple samples taken vertically.
            float2 uv = texelSize * (input.dispatchThreadID.xy + float2(0.2f, 0.5f));
            float2 offset = texelSize * float2(0.0f, 0.5f);

            sourceMipColor = 0.5f * (sourceMip.SampleLevel(linearClampSampler, uv, sourceMipLevel) + sourceMip.SampleLevel(linearClampSampler, uv + offset, sourceMipLevel));
            break;
        }

        case WIDTH_HEIGHT_ODD:
        {
            // 4 bilinear samples in vertical + horizontal directions.
            float2 uv = texelSize * (input.dispatchThreadID.xy + float2(0.25, 0.25));
            float2 offset = texelSize * 0.5;
 
            sourceMipColor = sourceMip.SampleLevel(linearClampSampler, uv, sourceMipLevel);
            sourceMipColor += sourceMip.SampleLevel(linearClampSampler, uv + float2(offset.x, 0.0f), sourceMipLevel);
            sourceMipColor += sourceMip.SampleLevel(linearClampSampler, uv + float2(0.0f, offset.y), sourceMipLevel);
            sourceMipColor += sourceMip.SampleLevel(linearClampSampler, uv + float2(offset.x, offset.y), sourceMipLevel);
            sourceMipColor *= 0.25f;

            break;
         }
    }
    
    outputMip1[input.dispatchThreadID.xy] = PackColor(sourceMipColor); 

    if (numberOutputMipLevels == 1)
    {
        return;
    }

    StoreColor(input.groupIndex, sourceMipColor);

    GroupMemoryBarrierWithGroupSync();
    
    // Check if width and height are even. (LOW 3 bits are for X, HIGH three bits are of Y).
    if ((input.groupIndex & 0x9) == 0)
    {
        float4 sourceColor2 = LoadColor(input.groupIndex + 0x01);
        float4 sourceColor3 = LoadColor(input.groupIndex + 0x08);
        float4 sourceColor4 = LoadColor(input.groupIndex + 0x09);
        sourceMipColor = 0.25 * (sourceMipColor + sourceColor2 + sourceColor3 + sourceColor4);

        outputMip2[input.dispatchThreadID.xy / 2] = PackColor(sourceMipColor);
 
        StoreColor(input.groupIndex, sourceMipColor);
    }

    if (numberOutputMipLevels == 2)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // Check if S and Y are multiples of 4.
    if ((input.groupIndex & 0x9) == 0)
    {
        float4 sourceColor2 = LoadColor(input.groupIndex + 0x02);
        float4 sourceColor3 = LoadColor(input.groupIndex + 0x10);
        float4 sourceColor4 = LoadColor(input.groupIndex + 0x12);
        sourceMipColor = 0.25 * (sourceMipColor + sourceColor2 + sourceColor3 + sourceColor4);

        outputMip2[input.dispatchThreadID.xy / 4] = PackColor(sourceMipColor);
 
        StoreColor(input.groupIndex, sourceMipColor);
    }

    if (numberOutputMipLevels == 3)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // Check if they are multiples of 8.
    if (input.groupIndex == 0)
    {
        float4 sourceColor2 = LoadColor(input.groupIndex + 0x04);
        float4 sourceColor3 = LoadColor(input.groupIndex + 0x20);
        float4 sourceColor4 = LoadColor(input.groupIndex + 0x24);
        sourceMipColor = 0.25 * (sourceMipColor + sourceColor2 + sourceColor3 + sourceColor4);

        outputMip2[input.dispatchThreadID.xy / 8] = PackColor(sourceMipColor);
 
        StoreColor(input.groupIndex, sourceMipColor);
    }
}