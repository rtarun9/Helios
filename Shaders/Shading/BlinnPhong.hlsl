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
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[renderResource.normalGBufferIndex];
    Texture2D<float4> positionTexture = ResourceDescriptorHeap[renderResource.positionGBufferIndex];

    float4 albedoColor = albedoTexture.Sample(pointClampSampler, psInput.textureCoord);

    float3 normal = normalTexture.Sample(pointClampSampler, psInput.textureCoord).xyz;
    float3 position = positionTexture.Sample(pointClampSampler, psInput.textureCoord).xyz;

    float3 outgoingLight = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(lightBuffer.lightPosition[i].xyz - position);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - position);

        // Ambient light.
        float ambientStrength = 0.00003f;
        float3 ambientColor = ambientStrength * lightBuffer.lightColor[i].xyz;

        // Diffuse light.
        float diffuseStrength = max(dot(normal, pixelToLightDirection), 0.0f);
        float3 diffuseColor =  diffuseStrength * lightBuffer.lightColor[i].xyz;
        bool isDiffuseZero = diffuseStrength <= MIN_FLOAT_VALUE;

        // Specular light.
        // lightDirection is negated as it is a vector from fragment to light, but the reflect function requires the opposite.
        float3 halfWayVector = normalize(viewDirection + pixelToLightDirection);
        
        float specularStrength = 0.6f;
        float shininessValue = 32.0f;

        float3 reflectionDirection = normalize(reflect(-pixelToLightDirection, normal));
        float3 specularColor = isDiffuseZero == true ? 0.0f : specularStrength * lightBuffer.lightColor[i].xyz * pow(max(dot(halfWayVector, normal), 0.0f), shininessValue);

        // Calculate light attenuation.
        float lightToPixelDistance = length(lightBuffer.lightPosition[i].xyz - position);

        outgoingLight += albedoColor.xyz * ambientColor + (diffuseColor + specularColor)  * albedoColor.xyz * 1.0f / pow(lightToPixelDistance, 2);
    }

    for (uint i = DIRECTIONAL_LIGHT_OFFSET; i < DIRECTIONAL_LIGHT_OFFSET + TOTAL_DIRECTIONAL_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(-lightBuffer.lightPosition[i].xyz);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - position);

        // Ambient light.
        float ambientStrength = 0.01f;
        float3 ambientColor = ambientStrength * lightBuffer.lightColor[i].xyz;

        // Diffuse light.
        float diffuseStrength = max(dot(normal, pixelToLightDirection), 0.0f);
        float3 diffuseColor =  diffuseStrength * lightBuffer.lightColor[i].xyz;

        // Specular light.
        // lightDirection is negated as it is a vector from fragment to light, but the reflect function requires the opposite.
        float3 halfWayVector = normalize(viewDirection + pixelToLightDirection);
        
        float specularStrength = 0.6f;
        float shininessValue = 32.0f;

        float3 reflectionDirection = normalize(reflect(-pixelToLightDirection, normal));
        float3 specularColor =  specularStrength * lightBuffer.lightColor[i].xyz * pow(max(dot(halfWayVector, normal), 0.0f), shininessValue);

        outgoingLight += albedoColor.xyz * ambientColor + (diffuseColor + specularColor)  * albedoColor.xyz;
    }

    return float4(outgoingLight.xyz, 1.0f);
}