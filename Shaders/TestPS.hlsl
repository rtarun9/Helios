#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
};

ConstantBuffer<TestRenderResources> renderResource : register(b0);

struct LightingData
{
    float4 lightPosition;
    float4 cameraPosition;
};

float4 PsMain(VSOutput input) : SV_Target
{
    Texture2D testTexture = ResourceDescriptorHeap[renderResource.textureIndex];
    ConstantBuffer<LightingData> lightCBuffer = ResourceDescriptorHeap[renderResource.lightCBufferIndex];

    float3 pixelToLightDir = normalize(lightCBuffer.lightPosition.xyz - input.worldSpacePosition.xyz);
    float3 viewDir = normalize(lightCBuffer.cameraPosition.xyz - input.worldSpacePosition.xyz);
    float3 halfWayDir = normalize(pixelToLightDir + viewDir);

    float angle = max(dot(input.normal, pixelToLightDir), 0.0f);

    float3 surfaceColor = testTexture.Sample(pointWrapSampler, input.texCoord).xyz;

    float3 ambientColor = surfaceColor * 0.1f;

    float specular = pow(max(dot(halfWayDir, input.normal), 0.0f), 128);

    float lightDistance = length(lightCBuffer.lightPosition.xyz - input.worldSpacePosition.xyz);

    float3 shadedColor = (specular * float3(1.0f, 1.0f, 1.0f) + surfaceColor * angle) + ambientColor;
    
    float4 result = float4(shadedColor, 1.0f);

    return result;
}