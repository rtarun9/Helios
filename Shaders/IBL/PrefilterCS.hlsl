#include "../Common/BindlessRS.hlsli"
#include "../Common/Utils.hlsli"

ConstantBuffer<PreFilterCubeMapRenderResources> renderResources : register(b0);

// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl.
// Why this shader is required.
// Unlike the diffuse part of the reflectance equation (where the variable terms that needed to be integrated was just integral(li(p, wi)*(n.wi)di)) (which only dependend on one vector wi, the incoming light direction from the scene).
// The specular part of the equation is integral(over hemisphere)ks * DFG / (4(wo.n)(wi.n)) * li(p, wi).n.widwi.
// or simplified, integral(over hemisphere)fr(p, wi, wo)li(p, wi).n.wi.dwi.
// We see, that the BRDF term requires both wi (incoming light direction) and wo(the view direction). Computing the integral for all incoming and outgoing light directions is not possible for real time rendering.
// So, we use Epic Game's split sum integral, to split the above integral into two parts.
// lo(p, wo) = (integral(over hemisphere)li(p, wi)dwi) * (integral(over hemisphere)fr(p, wi, wo).n.wi dwi).
// This shader (the prefiltered environment map) will solve the integral for the first half of the split sum integral.
// This shader will convolute the environment cube map taking roughness levels into account. More rough the surface, more blurrier the reflections would be.
// Sample vectors are generated at random and scattered using the NDF. As the NDF takes both normal and viewdirection, we approximate by taking view direction to be equal to output sample direction.

static const uint NUM_SAMPLES = 1024u;
static const float INV_NUM_SAMPLES = 1.0f / (float)NUM_SAMPLES;
static const uint PREFILTER_MIP_LEVELS = 6u;

// Using the VanDerCorput radical inverse along with HammersleySequence to get the low discrepensy sample i over total number of samples (NUM_SAMPLES).
float VanDerCorputRadicalInverse(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    return float(bits) * 2.3283064365386963e-10;
}

float2 SampleHammersleySequence(uint i)
{
    return float2((float) i * INV_NUM_SAMPLES, VanDerCorputRadicalInverse(i));
}

// Get sample vector based on Importance Sampling GGX normal distribution function for a fixed value of roughness.
// In specular IBL, we know that based on the roughness value, the only relavant sample vectors are those which lie in the specular lobe region.
// So, by restricting the region these sample vectors are generated, we can get a faster rate of convergence.
// Also note that the sample vectors will be oriented around the microfacet half way vector, and region is constrained on the roughness.
// Uses GGX normal distribution to return a normalized half way vector between li and lo. (not at random, but biased to be oriented towards microfacet normal). 
// Reference : https://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
// This uses the Trowbridge Reitz aka GGX NDF.
float3 SampleGGX(float2 u, float roughnessFactor)
{
    float alpha = pow(roughnessFactor, 2);
    
    float cosTheta = sqrt((1.0f - u.y) / (1.0f +  (pow(alpha, 2) - 1.0f) * u.y));
    float sinTheta = sqrt(1.0f - pow(cosTheta, 2.0f));
    
    float phi = TWO_PI * u.x;
    
    // The Spherical coordinate should be converted to cartesian coordinates before returning.
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Uses Disney's way of having alpha = roughness^2.
// Approximates the number of microfacts on the surface whose local normals are aligned with the half way vector. For light to reflect from the surface (diffuse or specular)
float NormalDistributionGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSquare = alpha * alpha;

    return alphaSquare / max(PI * pow((cosLh * cosLh) * (alphaSquare - 1.0f) + 1.0f, 2), MIN_FLOAT_VALUE);
}

float3 TangentToWorldCoords(float3 v, float3 n, float3 s, float3 t)
{
    return s * v.x + t * v.y + n * v.z;
}

// Calculates the integral Li(p, wi)dwi. For increasing levels of roughness (which is the mip - level), the environment is convoluted with much higher and roughly scattered sample vectors.
[numthreads(32, 32, 1)]
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
    
    float2 uv = (dispatchThreadID.xy + 0.5f) / float2(textureWidth, textureHeight);
    
    // For importance sampling, the solid angle associated with single cube map texel at base mip level will be useful.
    float wt = 4.0f * PI / (6.0f * inputTextureWidth * inputTextureHeight);

    float3 normal = GetSamplingVector(uv, dispatchThreadID);

    // Making the assumption that view direction = normal.
    float3 lo = normal;

    // Calculate orthonormal basis for conversion from tangent space -> world space.
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);

    ComputeBasisVectors(normal, s, t);
    
    float weight = 0.0f;
    float3 preFilteredColor = float3(0.0f, 0.0f, 0.0f);
    
    float roughness = renderResources.mipLevel / (PREFILTER_MIP_LEVELS - 1u);

    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        // Generate random vector that will be passed to SampleGGX.
        float2 u = SampleHammersleySequence(i);
        
        // Convert point from spherical coordinates to cartesian coordinates.
        float3 samplePoint = SampleGGX(u, roughness);
        float3 lh = TangentToWorldCoords(samplePoint, normal, s, t);
        
        // Get incident direction by reflecting view dir around half way vector.
        // The view direction is just the normal.
        float3 li = reflect(-lo, lh);
        
        float cosTheta = dot(lo, li);
        if (cosTheta > 0.0f)
        {
            float cosLh = saturate(dot(normal, lh));

            //  Calculate which miplevel of environment texture cube to sample from.
            // The division by 4 is becaues of density change in terms of lh to li (source : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl)
            float pdf = NormalDistributionGGX(cosLh, roughness) * 0.25f;
            
            // Solid angle for this sample.
            float ws = 1.0f / (NUM_SAMPLES * pdf);
            
            float mipLevel = max(0.5f * log2(ws / wt) + 1.0f, 0.0f);
                       
            preFilteredColor += textureCubeMap.SampleLevel(linearWrapSampler, li, mipLevel).rgb * cosTheta;
            weight += cosTheta;
        }
    }
    
    outputPreFilterCubeMap[dispatchThreadID] = float4(preFilteredColor / max(weight, MIN_FLOAT_VALUE), 1.0f);
}
