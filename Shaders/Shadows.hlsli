#ifndef __SHADOWS_HLSLI__
#define __SHADOWS_HLSLI__

float CalculateSimpleShadow(float4 lightSpaceWorldPosition, uint shadowDepthBufferIndex)
{
    float3 shadowPosition = lightSpaceWorldPosition.xyz / lightSpaceWorldPosition.z;
    shadowPosition.x = shadowPosition.x * 0.5f + 0.5f;
    shadowPosition.y = shadowPosition.y * -0.5f + 0.5f;

    if (shadowPosition.z > 1.0f)
    {
        return 0.0f;
    }

    Texture2D<float4> shadowDepthBuffer = ResourceDescriptorHeap[shadowDepthBufferIndex];
    float closestDepth = shadowDepthBuffer.Sample(linearClampSampler, shadowPosition.xy).x;

    return shadowPosition.z > closestDepth ? 1.0f : 0.0f;
}

#endif