#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float3 worldSpacePosition : WORLD_SPACE_POSITION;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];
    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    float4 albedoColor = GetAlbedo(psInput.textureCoord, renderResource.albedoTextureIndex);
    
    float3 outgoingLight = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 lightDirection = normalize(lightBuffer.lightPosition[i].xyz  - psInput.worldSpacePosition);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - psInput.worldSpacePosition);

        float3 reflectionDirection = normalize(reflect(-lightDirection, psInput.normal));

        float nDotL = pow(max(dot(reflectionDirection, viewDirection), 0.0fS), 64);
        
        float3 radiance = lightBuffer.lightColor[i].xyz;
        
        outgoingLight += albedoColor.xyz * radiance * nDotL;
    }

    outgoingLight /= (float)TOTAL_POINT_LIGHTS;

    return float4(outgoingLight, 1.0f);
}