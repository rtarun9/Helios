// clang-format off

#include "Utils.hlsli"
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : Texture_Coord;
};

ConstantBuffer<interlop::BlinnPhongRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f),
                                               float3(-1.0f, -3.0f, 0.0f)};

    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[clamp(0, 3, vertexID)], 1.0f);
    output.textureCoord = output.position.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return output;
}

[RootSignature(BindlessRootSignature)] 
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResources.albedoGBufferIndex];
    Texture2D<float4> normalEmissiveTexture = ResourceDescriptorHeap[renderResources.normalEmissiveGBufferIndex];
    Texture2D<float4> positionEmissiveTexture = ResourceDescriptorHeap[renderResources.positionEmissiveGBufferIndex];

    const float4 albedoColor = albedoTexture.Sample(pointClampSampler, input.textureCoord);

    float3 normal = normalEmissiveTexture.Sample(pointClampSampler, input.textureCoord).xyz;
    const float3 position = positionEmissiveTexture.Sample(pointClampSampler, input.textureCoord).xyz;

    // Ambient lighting.
    const float ambientStrength = 0.01;
    float3 ambientLight = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < interlop::TOTAL_LIGHTS; ++i)
    {
        ambientLight += albedoColor.xyz * ambientStrength * lightBuffer.lightColor[i].xyz; 

    }

    // Diffuse lighting.
    normal = normalize(normal);

    float3 diffuseLight = float3(0.0f, 0.0f, 0.0f);
    
    for (uint j = 0; j <interlop::TOTAL_POINT_LIGHTS; ++j)
    {
        const float3 pixelToLightDirection = normalize(lightBuffer.viewSpaceLightPosition[j].xyz - position.xyz);
        diffuseLight += max(dot(pixelToLightDirection, normal), 0.0f) * albedoColor.xyz * lightBuffer.lightColor[j].xyz;
    }

    // Specular lighting.
    float3 specularLight = float3(0.0f, 0.0f, 0.0f);
    for (uint k = 0; k < interlop::TOTAL_POINT_LIGHTS; ++k)
    {
        const float3 pixelToLightDirection = normalize(lightBuffer.viewSpaceLightPosition[k].xyz - position.xyz);

        const float3 perfectReflectionDirection = reflect(-pixelToLightDirection, normal);
        const float3 pixelToEyeDirection = normalize(-position.xyz);

        const float specularPower = 128.0f;
        const float3 halfWayVector = normalize(pixelToEyeDirection + pixelToLightDirection);
        
        const float specularIntensity = pow(max(dot(halfWayVector , normal), 0.0f), specularPower);
        const float specularStrength = 0.2f;

        specularLight += specularIntensity * specularStrength * lightBuffer.lightColor[k].xyz;
    }


    const float3 result = ambientLight + diffuseLight + specularLight;
    return float4(result, 1.0f);
}