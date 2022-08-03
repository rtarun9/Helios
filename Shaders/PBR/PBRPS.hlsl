#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];
    float4 albedoColor = GetAlbedo(psInput.textureCoord, renderResource.albedoTextureIndex);
    
    float3 outgoingLight = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < TOTAL_LIGHTS; ++i)
    {
        // If directional light.
        if (lightBuffer.lightPosition[i].w == 0.0f)
        {
            float3 lightDirection = normalize(-lightBuffer.lightPosition[i].xyz);
            float nDotL = saturate(dot(psInput.normal, lightDirection));

            float3 radiance = lightBuffer.lightColor[i].xyz;

            outgoingLight += albedoColor.xyz * radiance * nDotL;
        }

        // if positional light
        else if (lightBuffer.lightPosition[i].w == 1.0f)
        {
            float3 lightDirection = normalize(lightBuffer.lightPosition[i].xyz  - psInput.position.xyz);
            float nDotL = saturate(dot(psInput.normal, lightDirection));

            float3 radiance = lightBuffer.lightColor[i].xyz;

            outgoingLight += albedoColor.xyz * radiance * nDotL;
        }
    }

    return float4(outgoingLight, 1.0f);
}