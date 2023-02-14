#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

#include "Utils.hlsli"

ConstantBuffer<interlop::PreFilterRenderResources> renderResources : register(b0);

// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl.
// Why this shader is required:
// The specular portion of the cook torrence BRDF is given as follows:
// Lo(wo) = [Integral over hemisphere at point p](BRDF * Li(wi, p) * n.wi)dwi.
// Now, the specular portion of the BRDF is : (NDF) * F * G / (4 . (wi.n) . (wo.n))
// It is not possible to integrate over both wi and wo simulateneously. This is because unlike the diffuse term, the non constant terms also depend on the viewer direction wo.
// To simplify computation, we can use epic game's split sum integral. Basically, we split the integral into two parts.
// [Integral] (BRDF . Li(wi, p) . (wi.n))dwi = [Integral]Li(wi, p).dwi + [Integral](BRDF . (n.wi))dwi.
// This specular prefilter compute shader aims to solve the first component of the sum, the integral of Li(wi, p).
// We also take roughness into account. The more rough a surface is, the more blurred the specular prefilter texture would look, as samples are scattered in a more random
// fashion and are not concentrated. We will use mip maps to help with this (LOD 0 : Specular prefilter texture for very low roughness, higher LOD's -> higher roughness),

// We generate samples using the Normal Distribution Function. Since it depends on half way vector (which required view vector) and we do not know of that yet,
// we will assume sampling vector = normal = output view vector.

// Unlike diffuse irradiance, where we always want the samples to be uniformally distributed over a hemisphere, here we only care about reflections, which are by nature
// oriented close to the reflection vector (based on how rough the surface is). The size of the specular lobe (the confined region where rays will reflect of the surface)
// is based on the roughness of surface. Small specular lobe : smooth surface, more rough the surface gets more large the specular lobe becomes.
// Instead of wasted a lot of samples for wi, we will use importance sampling and give bias to those vectors that are within the confined specular lobe.
// I.E our montecarlo estimator will be biased here.
// We will use importance sampling to generate vectors that are confied by the specluar lobe and roughly aligned in the same direction as reflection direction.


static const uint NUM_SAMPLES = 2048u;
static const float INV_SAMPLES = 1.0f / (float)NUM_SAMPLES;
static const uint PREFILTER_MIP_LEVELS = 6u;

float vanDerCorputRadicalInverse(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}

// Get the i'th point from Hammersley sequence, a low discrepancy sequence.
float2 sampleHammersleySequence(uint i)
{
    return float2((float)i * INV_SAMPLES, vanDerCorputRadicalInverse(i));
}

// Get sample vector based on Importance Sampling GGX normal distribution function for a fixed value of roughness.
// In specular IBL, we know that based on the roughness value, the only relavant sample vectors are those which lie in the specular lobe region.
// So, by restricting the region these sample vectors are generated, we can get a faster rate of convergence.
// Also note that the sample vectors will be oriented around the microfacet half way vector, and region is constrained on the roughness.
// Uses GGX normal distribution to return a normalized half way vector between li and lo. (not at random, but biased to be oriented towards microfacet normal). 
//https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// This uses the Trowbridge Reitz aka GGX NDF.

// Returns a halfway vector between light direction and view direction (based on roughness). 
float3 importanceSamplingGGX(float2 uv, float roughnessFactor, float3 normal, float3 s, float3 t)
{
    const float alpha = pow(roughnessFactor, 2);
    
    const float phi = 2.0f * PI * uv.x;

    const float cosTheta = sqrt((1.0f - uv.y) / (1.0f +  (pow(alpha, 2) - 1.0f) * uv.y));
    const float sinTheta = sqrt(1.0f - pow(cosTheta, 2.0f));
    
    
    // Spherical -> Cartesian coordinates.
    const float3 h = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent to world space transformation.
    return h.x * s + h.y * t + normal * h.z;
}

float normalDistributionGGX(float nDotH, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSquare = alpha * alpha;

    return alphaSquare / max(PI * pow((nDotH * nDotH) * (alphaSquare - 1.0f) + 1.0f, 2), MIN_FLOAT_VALUE);
}

// Calculates the integral Li(p, wi)dwi. For increasing levels of roughness (which is the mip - level), the environment is convoluted with much higher and roughly scattered sample vectors.
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    TextureCube<float4> textureCubeMap = ResourceDescriptorHeap[renderResources.skyBoxTextureIndex];
    
    RWTexture2DArray<float4> outputPreFilterCubeMap = ResourceDescriptorHeap[renderResources.outputPreFilteredCubeMapIndex];
    
    float textureWidth, textureHeight, textureDepth;
    outputPreFilterCubeMap.GetDimensions(textureWidth, textureHeight, textureDepth);
    
    // Check to make sure that when calculating for higher mip levels we dont write past the output texture.
    if (dispatchThreadID.x >= textureWidth || dispatchThreadID.y >= textureHeight)
    {
        return;
    }
    
    float inputTextureWidth, inputTextureHeight, inputTextureMipLevels;
    textureCubeMap.GetDimensions(0u, inputTextureWidth, inputTextureHeight, inputTextureMipLevels);
    
    const float2 uv = dispatchThreadID.xy / float2(textureWidth, textureHeight);
    
    // For importance sampling, the solid angle associated with single cube map texel at base mip level will be used.
    const float wt = 4.0f * PI / ((float)PREFILTER_MIP_LEVELS * inputTextureWidth * inputTextureHeight);

    const float3 normal = normalize(getSamplingVector(uv, dispatchThreadID));

    // Making the assumption that view direction = normal.
    const float3 viewDirection = normal;

    // Calculate orthonormal basis for conversion from tangent space -> world space.
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);

    computeBasisVectors(normal, s, t);
    
    float weight = 0.0f;
    float3 preFilteredColor = float3(0.0f, 0.0f, 0.0f);
    
    const float roughness = (float)renderResources.mipLevel / (float)(PREFILTER_MIP_LEVELS);

    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        // Generate random vector uniformally.
        const float2 uv = sampleHammersleySequence(i);
        
        const float3 halfWayVector = importanceSamplingGGX(uv, roughness, normal, s, t);

        // Get incident direction by reflecting view dir around half way vector.
        // The view direction is just the normal.
        const float3 lightDirection = reflect(-viewDirection, halfWayVector);
        
        const float nDotL = dot(normal, lightDirection);
        if (nDotL > 0.0f)
        {
            // 'Technically' now that we have light direction, normal, and view direction, to solve for integral of Li(wi)dwi, we just need to sample the cube map using the light direction.
            // Below code is from a few sources and I do not understand it.
            // However, we figure out which mip level to use for the sampling. This is required because of high frequency components in the cube map (high / rapid changes in intensity).
            // Use Mipmap Filtered Importance Sampling to improve convergence.
            // See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4

            const float nDotH = saturate(dot(normal, halfWayVector));

            const float pdf = normalDistributionGGX(nDotH, roughness) * 0.25f;
            
            // Solid angle for this sample.
            const float ws = 1.0f / (NUM_SAMPLES * pdf);
            
            const float mipLevel = max(0.5f * log2(ws / wt) + 1.0f, 0.0f);
                       
            // No idea why that roughness check is required, but required for SUPER metallic materials.
            preFilteredColor += textureCubeMap.SampleLevel(linearClampSampler, lightDirection, roughness == 0 ? 0 : mipLevel).rgb * nDotL;
            weight += nDotL;
        }
    }
    
    outputPreFilterCubeMap[dispatchThreadID] = float4(preFilteredColor / max(weight, MIN_FLOAT_VALUE), 1.0f);
}