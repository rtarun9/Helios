#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3x3 modelMatrix : MODEL_MATRIX;
    float3 worldSpacePosition : WORLD_SPACE_POSITION;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];
    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    float4 albedoColor = GetAlbedo(psInput.textureCoord, renderResource.albedoTextureIndex, renderResource.albedoTextureSamplerIndex);
    if (albedoColor.a < 0.9f)
    {
        discard;
    }

    float3 outgoingLight = float3(0.0f, 0.0f, 0.0f);

    float3 normal = GetNormal(psInput.normal, psInput.textureCoord, psInput.tangent, psInput.modelMatrix, renderResource.normalTextureIndex, renderResource.normalTextureSamplerIndex);

    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 lightDirection = normalize(lightBuffer.lightPosition[i].xyz - psInput.worldSpacePosition);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - psInput.worldSpacePosition);

        // Diffuse light.
        float3 diffuseColor = max(dot(normal, lightDirection), 0.0f) * lightBuffer.lightColor[i].xyz;

        // Ambient light.
        float ambientStrength = 0.01f;
        float3 ambientColor = ambientStrength * lightBuffer.lightColor[i].xyz;

        // Specular light.
        // lightDirection is negated as it is a vector from fragment to light, but the reflect function requires the opposite.
        float3 halfWayVector = normalize(lightDirection + viewDirection);
        float specularStrength = 0.5f;
        float3 reflectionDirection = reflect(-lightDirection, normal);
        float3 specularColor = specularStrength * lightBuffer.lightColor[i].xyz * pow(max(dot(halfWayVector, psInput.normal), 0.0f), 32);

        // Calculate light attenuation.
        float lightToPixelDistance = length(lightBuffer.lightPosition[i].xyz - psInput.worldSpacePosition.xyz);
        float attenuation = 1.0f / lightToPixelDistance;

        outgoingLight += (ambientColor + diffuseColor + specularColor) * albedoColor.xyz;// *attenuation;
    }

    outgoingLight /= (float)TOTAL_POINT_LIGHTS;

    return float4(outgoingLight, 1.0f);
}