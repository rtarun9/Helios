// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/renderResources.hlsli"
#include "Utils.hlsli"
#include "Shadow/PCFShadows.hlsli"
#include "Shading/BRDF.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    matrix lightSpaceMatrix : LIGHT_SPACE_MATRIX;
};

ConstantBuffer<interlop::PBRRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] VSOutput VsMain(uint vertexID
                                                       : SV_VertexID) {
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f),
                                               float3(-1.0f, -3.0f, 0.0f)};

    ConstantBuffer<interlop::ShadowBuffer> shadowBuffer = ResourceDescriptorHeap[renderResources.shadowBufferIndex];
    
    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[vertexID], 1.0f);
    output.textureCoord = output.position.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    output.lightSpaceMatrix = shadowBuffer.lightViewProjectionMatrix;

    return output;
}

    [RootSignature(BindlessRootSignature)] float4 PsMain(VSOutput psInput)
    : SV_Target
{
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    const matrix inverseViewMatrix = sceneBuffer.inverseViewMatrix;

    // Sample and extract data for the GBuffer's.
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[renderResources.albedoGBufferIndex];
    Texture2D<float4> positionEmissiveTexture = ResourceDescriptorHeap[renderResources.positionEmissiveGBufferIndex];
    Texture2D<float4> normalEmissiveTexture = ResourceDescriptorHeap[renderResources.normalEmissiveGBufferIndex];
    Texture2D<float4> aoMetalRoughnessEmissiveTexture =
        ResourceDescriptorHeap[renderResources.aoMetalRoughnessEmissiveGBufferIndex];


    TextureCube<float4> irradianceTexture = ResourceDescriptorHeap[renderResources.irradianceTextureIndex];
    TextureCube<float4> preFilterTexture = ResourceDescriptorHeap[renderResources.prefilterTextureIndex];
    Texture2D<float2> brdfLUTTexture = ResourceDescriptorHeap[renderResources.brdfLUTTextureIndex];

    Texture2D<float> blurredSSAOTexture = ResourceDescriptorHeap[renderResources.blurredSSAOTextureIndex];

    const float4 albedo = albedoTexture.Sample(pointClampSampler, psInput.textureCoord);

    const float4 positionEmissive = positionEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);
    const float4 normalEmissive = normalEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);
    const float4 aoMetalRoughnessEmissive =
        aoMetalRoughnessEmissiveTexture.Sample(pointClampSampler, psInput.textureCoord);

    const float3 viewSpacePosition = positionEmissive.xyz;
    const float3 normal = normalize(normalEmissive.xyz);

    const float ao = aoMetalRoughnessEmissive.r;
    const float metallicFactor = aoMetalRoughnessEmissive.g;
    const float roughnessFactor = aoMetalRoughnessEmissive.b;

    const float3 emissive = float3(positionEmissive.w, normalEmissive.w, aoMetalRoughnessEmissive.a);

    const float3 viewDirection = normalize(-viewSpacePosition);

    const float ssaoTerm = blurredSSAOTexture.Sample(linearWrapSampler, psInput.textureCoord);
    // Reflectance equation for reference.
    // lo(x, v) = le(x, v) + integral(over hemisphere centered at x)(fr(x, l, v, roughness) * li(x, l) * (l.n)dl
    // x is the pixel position
    // v is the view direction
    // l is the light vector
    // lo is the irradiance, while li is the light radiance
    // fr is the brdf term.

    float3 lo = float3(0.0f, 0.0f, 0.0f);


    for (uint i = 0; i < lightBuffer.numberOfLights; ++i)
    {
        float3 pixelToLightDirection = normalize(lightBuffer.viewSpaceLightPosition[i].xyz - viewSpacePosition);

        const float distance = length(lightBuffer.viewSpaceLightPosition[i].xyz - viewSpacePosition);
        float attenuation = 1.0f / (distance * distance);

        // Check if we are dealing with directional light. Directional light is always at index 0.
        if (i == 0u)
        {
            pixelToLightDirection = normalize(-lightBuffer.viewSpaceLightPosition[i].xyz);
            attenuation = 1.0f;
            
            // Since this is the directional light, the shading calculation must take into account shadow computation.
            const float3 worldSpaceNormal = normalize(mul(normal, (float3x3)inverseViewMatrix));
            const float4 worldSpacePosition = mul(float4(viewSpacePosition, 1.0f), inverseViewMatrix);
            
            const float3 worldPixelToLightDirection = normalize(-lightBuffer.lightPosition[0].xyz);
            
            const float nDotL = saturate(dot(worldSpaceNormal, worldPixelToLightDirection));
            
            const float4 lightSpacePosition = mul(worldSpacePosition, psInput.lightSpaceMatrix);
            const float shadow = calculateShadow(lightSpacePosition, nDotL, renderResources.shadowDepthTextureIndex);

            attenuation = (1.0f - shadow);
        }

        const float3 brdf =
            cookTorrenceBRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);
        const float3 radiance = lightBuffer.lightColor[i].xyz * lightBuffer.radiusIntensity[i].y;

        lo += brdf * radiance * saturate(dot(pixelToLightDirection, normal)) * attenuation;
    }
    
    // Calculate ambient lighting from irradiance map.
    
    const float3 worldSpaceNormal = normalize(mul(normal, (float3x3)inverseViewMatrix));
    const float3 reflectionDirection = normalize(mul(reflect(-viewDirection, normal), (float3x3)inverseViewMatrix));


    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metallicFactor);
    
    const float3 kS = fresnelSchlickFunctionRoughness(f0, saturate(dot(viewDirection, normal)), roughnessFactor);
    const float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), metallicFactor);
    
    const float3 irradiance = irradianceTexture.Sample(linearClampSampler, worldSpaceNormal).rgb;
    const float3 diffuseIBL = kD * irradiance * albedo.xyz / (float)PI;

    const float3 specularPreFilter = preFilterTexture.SampleLevel(minMapLinearMipPointClampSampler, reflectionDirection, roughnessFactor * 6.0f).xyz;
    
    const float nDotV = saturate(dot(viewDirection, normal));
    const float2 brdfLut = brdfLUTTexture.Sample(pointWrapSampler, float2(nDotV, roughnessFactor)).xy;

    const float3 specularIBL = specularPreFilter  * (f0 * brdfLut.x  + brdfLut.y);

    float3 ambient = (diffuseIBL + specularIBL) * ao * ssaoTerm;

    lo += emissive + ambient;

    return float4(lo, 1.0f);

}