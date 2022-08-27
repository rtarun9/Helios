#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<DeferredLightingPassRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float2> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordsBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];

    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xy, 0.0f, 1.0f);
    output.textureCoord = textureCoordsBuffer[vertexID];

    return output;
}

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];
    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResource.albedoGBufferIndex];
    Texture2D<float4> normalEmissiveTexture = ResourceDescriptorHeap[renderResource.normalEmissiveGBufferIndex];
    Texture2D<float4> positionEmissiveTexture = ResourceDescriptorHeap[renderResource.positionEmissiveGBufferIndex];

    float4 albedoColor = albedoTexture.Sample(pointClampSampler, psInput.textureCoord);

    float3 normal = normalEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord).xyz;
    float3 position = positionEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord).xyz;

    float3 outgoingLight = float3(0.0f, 0.0f, 0.0f);

    // Gooch model equation : 
    // c(shaded) = s*c(highlight) + (1 - s)(tc(warm) + (1 - t)c(cool)).
    float3 coolShade = float3(0.0f, 0.0f, 0.55f) + 0.25f * albedoColor.xyz;
    float3 warmShade = float3(0.3f, 0.3f, 0.0f) + 0.25f * albedoColor.xyz;
    float3 highLight = float3(1.0f, 1.0f, 1.0f);

    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(lightBuffer.lightPosition[i].xyz - position);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - position);

        float3 t = (max(dot(pixelToLightDirection, normal), 0.0));
        float3 r = normalize(reflect(-pixelToLightDirection, normal));
        float3 s = saturate(100 * max(dot(r, viewDirection), 0.0f) - 97);

        // Calculate light attenuation.
        float lightToPixelDistance = length(lightBuffer.lightPosition[i].xyz - position);

        outgoingLight += (s * highLight + (1 - s) * (t * warmShade + (1 - t) * coolShade)) *  1.0f / pow(lightToPixelDistance, 2);;
    }

    for (uint i = DIRECTIONAL_LIGHT_OFFSET; i < DIRECTIONAL_LIGHT_OFFSET + TOTAL_DIRECTIONAL_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(-lightBuffer.lightPosition[i].xyz);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - position);
       
    
        float3 t = (max(dot(pixelToLightDirection, normal), 0.0f) + 1) * 0.5f;
        float3 r = normalize(reflect(-pixelToLightDirection, normal));
        float3 s = saturate(100 * max(dot(r, viewDirection), 0.0f) - 97);

        outgoingLight += s * highLight + (1 - s) * (t * warmShade + (1 - t) * coolShade);
    }

    return float4(outgoingLight.xyz, 1.0f);
}