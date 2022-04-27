#include "BindlessRS.hlsli"

// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl.

ConstantBuffer<PreFilterCubeMapRenderResources> renderResources : register(b0);

static const float PI = 3.14159265359;

static const float NUM_SAMPLES = 2048.0f;
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
float3 SampleGGX(float u1, float u2, float roughness)
{
    float alpha = pow(roughness, 2);
    
    float cosTheta = sqrt((1.0f - u2) / (1.0f +  (pow(alpha, 2) - 1.0f) * u2));
    float sinTheta = sqrt(1.0f - pow(cosTheta, 2.0f));
    
    // The Spherical coordinate should be converted to cartesian coordinates before returning.
    float phi = PI * 2.0f * u1;
    
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float NDFGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

[numthreads(32, 32, 1)]
void CsMain(uint3 threadID : SV_DispatchThreadID)
{
     TextureCube<float4> textureCubeMap = ResourceDescriptorHeap[renderResources
    .textureCubeMapIndex];
    
    RWTexture2DArray<float4> outputPreFilterCubeMap = ResourceDescriptorHeap[renderResources.outputPreFilteredCubeMapIndex];
    
    float textureWidth, textureHeight, textureDepth;
    outputPreFilterCubeMap.GetDimensions(textureWidth, textureHeight, textureDepth);
    
    // Check to make sure that when calculating for higher mip levels we dont write past the output texture.
    if (threadID.x >= textureWidth || threadID.y >= textureHeight)
    {
        return;
    }
    
    float inputTextureWidth, inputTextureHeight, inputTextureMipLevels;
    textureCubeMap.GetDimensions(0u, inputTextureWidth, inputTextureHeight, inputTextureMipLevels);
    
    float roughness = renderResources.mipLevel / 5.0f;
    
    float2 uv = float2(threadID.xy) / float2(textureWidth, textureHeight);
    uv = 2.0f * float2(uv.x, 1.0f - uv.y) - float2(1.0f, 1.0f);
    
    float3 samplingVector = float3(0.0f, 0.0f, 0.0f);
    
    switch (threadID.z)
    {
        case 0:
            samplingVector = float3(1.0, uv.y, -uv.x);
            break;
        case 1:
            samplingVector = float3(-1.0, uv.y, uv.x);
            break;
        case 2:
            samplingVector = float3(uv.x, 1.0, -uv.y);
            break;
        case 3:
            samplingVector = float3(uv.x, -1.0, uv.y);
            break;
        case 4:
            samplingVector = float3(uv.x, uv.y, 1.0);
            break;
        case 5:
            samplingVector = float3(-uv.x, uv.y, -1.0);
            break;
    }
    
    samplingVector = normalize(samplingVector);
   
    // Solid angle that corresponds to cube map texel at mip level zero.
    float wt = 4.0f * PI / (6.0f * inputTextureWidth * inputTextureHeight);
    
    float3 normal = samplingVector;
    float3 L0 = normalize(normal);

    // Calculate orthonormal basis for conversion from tangent space -> world space.
    float3 T = cross(normal, float3(0.0f, 1.0f, 0.0f));
    T = lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), T, step(0.00001f, dot(T, T)));
    
    T = normalize(T);   
    float3 S = normalize(cross(normal, T));

    float weight = 0.0f;
    float3 preFilteredColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        float2 u = SampleHammersleySequence(i);
        
        // Convert point from tanget to world space.
        float3 samplePoint = SampleGGX(u.x, u.y, roughness);
        float3 Lh = S * samplePoint.x + T * samplePoint.y + normal * samplePoint.z;
        
        // Get incident direction by reflecting view dir around half way vector.
        float3 Li = normalize(2.0f * dot(L0, Lh) * Lh - L0);
        
        float cosTheta = saturate(dot(L0, Li));
        if (cosTheta > 0.0f)
        {
            float cosLH = max(dot(normal, Lh), 0.0f);
            
            float pdf = NDFGGX(cosLH, roughness) * 0.25f;
            
            float ws = 1.0f / (NUM_SAMPLES * pdf);
            
            float mipLevel = max(0.5f * log2(ws / wt) + 1.0f, 0.0f);
           
            preFilteredColor += textureCubeMap.SampleLevel(linearClampSampler, Li, mipLevel).rgb * cosTheta;
            weight += cosTheta;
        }
    }
    
    outputPreFilterCubeMap[threadID] = float4(preFilteredColor / max(weight, 0.001f), 1.0f);

}