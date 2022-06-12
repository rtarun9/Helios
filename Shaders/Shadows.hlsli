#ifndef __SHADOWS_HLSLI__
#define __SHADOWS_HLSLI__

#include "ConstantBuffers.hlsli"

static const float NEAR_PLANE = 0.5f;
static const float FAR_PLANE = 1000.0f;
static const float CSM_CASCADES[6] = { NEAR_PLANE, FAR_PLANE / 50.0f, FAR_PLANE / 25.0f, FAR_PLANE / 10.0f, FAR_PLANE / 2.0f, FAR_PLANE };

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

float CalculateCSMShadow(float4 lightSpaceWorldPosition, matrix viewMatrix, uint csmShadowDepthBufferStartingIndex, uint csmShadowMappingDataBufferStartingIndex)
{
    float4 viewSpaceWorldPosition = mul(lightSpaceWorldPosition, viewMatrix);
    float depth = abs(viewSpaceWorldPosition.z);
    
    uint layer = -1;
    for (uint i = 0; i < 6; ++i)
    {
        if (depth < CSM_CASCADES[i])
        {
            layer = i;
            break;
        }
    }
    
    if (layer == -1)
    {
        layer = 6;
    }
    
    ConstantBuffer<TransformData> mvpCBuffer = ResourceDescriptorHeap[csmShadowMappingDataBufferStartingIndex + layer];
    
    float4 lightSpacePosition = mul(lightSpaceWorldPosition, mul(mvpCBuffer.viewMatrix, mvpCBuffer.projectionMatrix));

    return CalculateSimpleShadow(lightSpacePosition, csmShadowDepthBufferStartingIndex + layer);
}
#endif