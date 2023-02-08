// clang-format off

#include "Utils.hlsli"
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 viewSpacePosition : VIEW_SPACE_POSITION;
    float3 viewSpaceNormal : VIEW_SPACE_NORMAL;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<interlop::BlinnPhongRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID: SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResources.textureCoordBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResources.normalBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResources.transformBufferIndex];
    
    VSOutput output;

    output.position = mul(mul(float4(positionBuffer[vertexID].xyz, 1.0f), transformBuffer.modelMatrix), sceneBuffer.viewProjectionMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.viewSpacePosition = mul(mul(float4(positionBuffer[vertexID].xyz, 1.0f), transformBuffer.modelMatrix), sceneBuffer.viewMatrix);
    output.viewSpaceNormal = mul(normalBuffer[vertexID].xyz, (float3x3)transpose(transformBuffer.inverseModelViewMatrix));

    return output;
}

[RootSignature(BindlessRootSignature)] 
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];

    const float4 albedoColor = getAlbedo(input.textureCoord, renderResources.albedoTextureIndex, renderResources.albedoTextureSamplerIndex);

    // Ambient lighting.
    const float ambientStrength = 0.01;
    float3 ambientLight = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < interlop::TOTAL_LIGHTS; ++i)
    {
        ambientLight += albedoColor.xyz * ambientStrength * lightBuffer.lightColor[i].xyz; 

    }

    // Diffuse lighting.
    const float3 normal = normalize(input.viewSpaceNormal);
    float3 diffuseLight = float3(0.0f, 0.0f, 0.0f);
    
    for (uint j = 0; j <interlop::TOTAL_POINT_LIGHTS; ++j)
    {
        const float3 pixelToLightDirection = normalize(lightBuffer.viewSpaceLightPosition[j].xyz - input.viewSpacePosition.xyz);
        diffuseLight += max(dot(pixelToLightDirection, normal), 0.0f) * albedoColor.xyz * lightBuffer.lightColor[j].xyz;
    }

    // Specular lighting.
    float3 specularLight = float3(0.0f, 0.0f, 0.0f);
    for (uint k = 0; k < interlop::TOTAL_POINT_LIGHTS; ++k)
    {
        const float3 pixelToLightDirection = normalize(lightBuffer.viewSpaceLightPosition[k].xyz - input.viewSpacePosition.xyz);

        const float3 perfectReflectionDirection = reflect(-pixelToLightDirection, normal);
        const float3 pixeToEyeDirection = normalize(-input.viewSpacePosition.xyz);

        const float specularPower = 32.0f;
        const float specularIntensity = pow(max(dot(perfectReflectionDirection, pixeToEyeDirection), 0.0f), specularPower);
        const float specularStrength = 0.5f;

        specularLight += specularIntensity * specularStrength * lightBuffer.lightColor[k].xyz;
    }


    const float3 result = ambientLight + diffuseLight + specularLight;
    return float4(result, 1.0f);
}