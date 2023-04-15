// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/renderResources.hlsli"
#include "Utils.hlsli"
#include "Shadow/PCFShadows.hlsli"
#include "Shading/BRDF.hlsli"


ConstantBuffer<interlop::PBRRenderResources> renderResources : register(b0);

    [RootSignature(BindlessRootSignature)] [numThreads(12, 8, 1)] void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::ShadowBuffer> shadowBuffer = ResourceDescriptorHeap[renderResources.shadowBufferIndex];

    // Sample and extract data for the GBuffer's.
    Texture2D<float4> albedoEmissiveTexture = ResourceDescriptorHeap[renderResources.albedoEmissiveGBufferIndex];
    Texture2D<float4> normalEmissiveTexture = ResourceDescriptorHeap[renderResources.normalEmissiveGBufferIndex];
    Texture2D<float4> aoMetalRoughnessEmissiveTexture =
        ResourceDescriptorHeap[renderResources.aoMetalRoughnessEmissiveGBufferIndex];

    TextureCube<float4> irradianceTexture = ResourceDescriptorHeap[renderResources.irradianceTextureIndex];
    TextureCube<float4> preFilterTexture = ResourceDescriptorHeap[renderResources.prefilterTextureIndex];
    Texture2D<float2> brdfLUTTexture = ResourceDescriptorHeap[renderResources.brdfLUTTextureIndex];

    Texture2D<float> blurredSSAOTexture = ResourceDescriptorHeap[renderResources.blurredSSAOTextureIndex];
    
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];

    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    float2 screenDimensions = float2(0.0f, 0.0f);
    albedoEmissiveTexture.GetDimensions(screenDimensions.x, screenDimensions.y);

    const float2 uv = (dispatchThreadID.xy + 0.5f) * 1.0f / screenDimensions;

    const float currentDepthValue = depthTexture.Sample(pointClampSampler, uv);
    const float4 albedoEmissive = albedoEmissiveTexture.Sample(pointClampSampler, uv);
    const float4 normalEmissive = normalEmissiveTexture.Sample(pointClampSampler, uv);

    const float4 aoMetalRoughnessEmissive =
        aoMetalRoughnessEmissiveTexture.Sample(pointClampSampler, uv);
    
    const float ssaoTerm = blurredSSAOTexture.Sample(linearClampSampler, uv) * aoMetalRoughnessEmissive.r;
       
    const float3 viewSpacePosition = viewSpaceCoordsFromDepthBuffer(currentDepthValue, uv, sceneBuffer.inverseProjectionMatrix);;//positionEmissive.xyz;
    const float3 albedo = albedoEmissive.xyz;

    const float3 normal = normalize(normalEmissive.xyz);

    const float metallicFactor = aoMetalRoughnessEmissive.g;
    const float roughnessFactor = aoMetalRoughnessEmissive.b;

    const float3 emissive = float3(albedoEmissive.w, normalEmissive.w, aoMetalRoughnessEmissive.a);

    const float3 viewDirection = normalize(-viewSpacePosition);

    const float3 worldSpaceNormal = normalize(mul(normal, (float3x3)sceneBuffer.inverseViewMatrix));
    
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
            const float4 worldSpacePosition = mul(float4(viewSpacePosition, 1.0f), sceneBuffer.inverseViewMatrix);
            
            const float3 worldPixelToLightDirection = normalize(-lightBuffer.lightPosition[0].xyz);
            
            const float nDotL = saturate(dot(worldSpaceNormal, worldPixelToLightDirection));
            
            const float4 lightSpacePosition = mul(worldSpacePosition, shadowBuffer.lightViewProjectionMatrix);
            const float shadow = calculateShadow(lightSpacePosition, nDotL, renderResources.shadowDepthTextureIndex);

            attenuation = (1.0f - shadow);
        }

        const float3 brdf =
            cookTorrenceSpecularBRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor) +
            lambertianDiffuseBRDF(normal, viewDirection, pixelToLightDirection, albedo.xyz, roughnessFactor, metallicFactor);

        lo += brdf * lightBuffer.lightColor[i].xyz * lightBuffer.radiusIntensity[i].y * saturate(dot(pixelToLightDirection, normal)) * attenuation;
    }
    
    // Calculate ambient lighting from irradiance map.
    
    const float3 reflectionDirection = normalize(mul(reflect(-viewDirection, normal), (float3x3)sceneBuffer.inverseViewMatrix));

    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metallicFactor);
    
    const float3 kS = fresnelSchlickFunctionRoughness(f0, saturate(dot(viewDirection, normal)), roughnessFactor);
    const float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), metallicFactor);
    
    const float nDotV = saturate(dot(viewDirection, normal));
    
    // Batching texture fetches for optimal performance.
    const float3 irradiance = irradianceTexture.Sample(linearClampSampler, worldSpaceNormal).rgb;
    const float3 specularPreFilter = preFilterTexture.SampleLevel(minMapLinearMipPointClampSampler, reflectionDirection, roughnessFactor * 6.0f).xyz;
    const float2 brdfLut = brdfLUTTexture.Sample(pointWrapSampler, float2(nDotV, roughnessFactor)).xy;

    const float3 diffuseIBL = kD * irradiance * albedo.xyz;
    const float3 specularIBL = specularPreFilter  * (f0 * brdfLut.x  + brdfLut.y);

    float3 ambient = (diffuseIBL + specularIBL) * ssaoTerm;

    lo += emissive + ambient;

    outputTexture[dispatchThreadID.xy] = float4(lo, 1.0f);

}