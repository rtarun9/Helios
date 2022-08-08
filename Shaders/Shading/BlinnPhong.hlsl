#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

#include "../Common/Resources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 biTangent : BITANGENT;
    float3 worldSpacePosition : WORLD_SPACE_POSITION;
    float3x3 tbnMatrix : TBN_MATRIX;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResource.normalBufferIndex];

    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];
    ConstantBuffer<TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    matrix mvpMatrix = mul(mul(transformBuffer.modelMatrix, sceneBuffer.viewMatrix), sceneBuffer.projectionMatrix);
    float3x3 normalMatrix = (float3x3)transpose(transformBuffer.inverseModelMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.normal = normalBuffer[vertexID];
    output.tangent = GenerateTangent(output.normal).xyz;
    output.biTangent = normalize(cross(output.normal, output.tangent));
    output.worldSpacePosition = mul(float4(positionBuffer[vertexID], 1.0f), transformBuffer.modelMatrix).xyz;
    
    // Calculation of tbn matrix.
    float3 t = normalize(mul(output.tangent, normalMatrix));
    float3 b = normalize(mul(output.biTangent, normalMatrix));
    float3 n = normalize(mul(output.normal, normalMatrix));

    output.tbnMatrix = float3x3(t, b, n);

    return output;
}


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

    float3 normal = GetNormal(psInput.textureCoord, renderResource.normalTextureIndex, renderResource.normalTextureSamplerIndex, psInput.normal, psInput.biTangent, psInput.tangent, psInput.tbnMatrix);
   
    for (uint i = 0; i < TOTAL_POINT_LIGHTS; ++i)
    {
        float3 pixelToLightDirection = normalize(lightBuffer.lightPosition[i].xyz - psInput.worldSpacePosition);
        float3 viewDirection = normalize(sceneBuffer.cameraPosition - psInput.worldSpacePosition);

        // Ambient light.
        float ambientStrength = 0.005f;
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
        float lightToPixelDistance = length(lightBuffer.lightPosition[i].xyz - psInput.worldSpacePosition.xyz);

        outgoingLight += ambientColor + (diffuseColor + specularColor)  * albedoColor.xyz ;//* PointLightAttenuationWithoutSingularity(lightToPixelDistance, lightBuffer.radius[i]);
    }

    outgoingLight /= (float)TOTAL_POINT_LIGHTS;

    return float4(normal, 1.0f);
}