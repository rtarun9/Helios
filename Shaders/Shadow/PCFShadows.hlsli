#pragma once

// Reference : https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping.
float calculateShadow(float4 lightSpaceWorldPosition, float nDotL, uint shadowDepthBufferIndex)
{
    // Do perspective divide
    float3 shadowPosition = lightSpaceWorldPosition.xyz / lightSpaceWorldPosition.w;

    if (shadowPosition.z > 1.0f)
    {
        return 0.0f;
    }

    // Convert x and y coord from [-1, 1] range to [0, 1].
    shadowPosition.x = shadowPosition.x * 0.5f + 0.5f;
    shadowPosition.y = shadowPosition.y * -0.5f + 0.5f;

    Texture2D<float> shadowDepthBuffer = ResourceDescriptorHeap[shadowDepthBufferIndex];

    // Get texture dimensions for texel size.
    float textureWidth, textureHeight;
    shadowDepthBuffer.GetDimensions(textureWidth, textureHeight);

    float2 texelSize = float2(1.0f, 1.0f) / float2(textureWidth, textureHeight);
    const float bias = max(0.05f * (1.0f - nDotL), 0.005f);

    // Do PCF.
    float shadow = 0.0f;

    // Half kernel width.
    const int samplesOffset = 2;
    for (int x = -samplesOffset; x <= samplesOffset; ++x)
    {
        for (int y = -samplesOffset; y <= samplesOffset; ++y)
        {
            float currPCFDepth =
                shadowDepthBuffer.Sample(pointClampSampler, shadowPosition.xy + float2(x, y) * texelSize).r;
            shadow += shadowPosition.z - bias > currPCFDepth ? 1.0f : 0.0f;
        }
    }

    return shadow / (float)(pow(samplesOffset * 2 + 1, 2));
}
