#ifndef __UTILS_HLSLI__
#define __UTILS_HLSLI__

float4 GetAlbedo(float2 texCoord, uint albedoTextureIndex)
{
    if (albedoTextureIndex == -1)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];
    return albedoTexture.Sample(anisotropicSampler, texCoord);
}

#endif