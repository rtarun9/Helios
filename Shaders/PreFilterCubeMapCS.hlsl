#include "BindlessRS.hlsli"
#include "BRDF.hlsli"
#include "Utils.hlsli"
// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl.

ConstantBuffer<PreFilterCubeMapRenderResources> renderResources : register(b0);

static const float NUM_SAMPLES = 1240.0f;
static const float INV_NUM_SAMPLES = 1.0f / NUM_SAMPLES;

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
// The function returns a half vector between Li and Lo.
// Used inorder to orient sample vector towards specular lobe of surface roughness.
float3 SampleGGX(float2 u, float roughness)
{
    float alpha = pow(roughness, 2);
    
    float cosTheta = sqrt((1.0f - u.y) / (1.0f +  (pow(alpha, 2) - 1.0f) * u.y));
    float sinTheta = sqrt(1.0f - pow(cosTheta, 2.0f));
    
    float phi = PI * 2.0f * u.x;
    
    // The Spherical coordinate should be converted to cartesian coordinates before returning.
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Calculates the integral Li(p, wi)dwi. For increasing levels of roughness (which is the mip - level), the environment is convoluted with much higher and roughly scattered sample vectors.
[numthreads(32, 32, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
     TextureCube<float4> textureCubeMap = ResourceDescriptorHeap[renderResources
    .textureCubeMapIndex];
    
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
    
    float roughness = renderResources.mipLevel / 5.0f;
    
    float2 uv = float2(dispatchThreadID.xy) / float2(textureWidth, textureHeight);
    float3 samplingVector = GetSamplingVector(uv, dispatchThreadID);
   
    // Solid angle that corresponds to cube map texel at mip level zero.
    float wt = 4.0f * PI / (6.0f * inputTextureWidth * inputTextureHeight);
    
    float3 normal = samplingVector;
    float3 lo = normalize(normal);

    // Calculate orthonormal basis for conversion from tangent space -> world space.
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);

    GenerateBasisFromVector(normal, t, s);
    
    float weight = 0.0f;
    float3 preFilteredColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        // Generate sample vectors biased towards the reflection orientation of half way vector.
        float2 u = SampleHammersleySequence(i);
        
        // Convert point from spherical coordinates to cartesian coordinates.
        float3 samplePoint = SampleGGX(u, roughness);
        float3 lh = TangentSpaceToWorldSpace(samplePoint, normal, t, s);
        
        // Get incident direction by reflecting view dir around half way vector.
        float3 li = (2.0f * dot(lo, lh) * lh - lo);
        
        float cosTheta = dot(lo, li);
        if (cosTheta > 0.0f)
        {
            //  Calculate which miplevel of environment texture cube to sample from.
            float pdf = GGXNormalDistribution(normal, lh, roughness) * 0.25f;
            
            float ws = 1.0f / (NUM_SAMPLES * pdf);
            
            float mipLevel = max(0.5f * log2(ws / wt) + 1.0f, 0.0f);
                       
            // The logic for sampling from a mip map is present, but currently mip map generation is not done. When it is done, the below code can be uncommented.
            // preFilteredColor += textureCubeMap.SampleLevel(linearWrapSampler, li, mipLevel).rgb * cosTheta;
            preFilteredColor += textureCubeMap.SampleLevel(linearWrapSampler, li, 0.0f).rgb * cosTheta;
            weight += cosTheta;
        }
    }
    
    outputPreFilterCubeMap[dispatchThreadID] = float4(preFilteredColor / max(weight, MIN_FLOAT_VALUE), 1.0f);

}