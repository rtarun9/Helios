#include "BindlessRS.hlsli"

// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl.

ConstantBuffer<BRDFConvolutionRenderResources> renderResources : register(b0);

static const float PI = 3.14159265359;

static const float NUM_SAMPLES = 2048.0f;
static const float INV_NUM_SAMPLES = 1.0f / NUM_SAMPLES;

struct LUTCBuffer
{
    float lutIndex;
};

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
    
    float cosTheta = sqrt((1.0f - u2) / (1.0f + (pow(alpha, 2) - 1.0f) * u2));
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

float SchlickGGX(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0f - k) + k);
}

// SchlickGGX approximation of geometric attenuation (using Smith's method).
float SchlickGGX(float cosLI, float cosLO, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0f;
    
    return SchlickGGX(cosLI, k) * SchlickGGX(cosLO, k);
}
[numthreads(32, 32, 1)]
void CsMain(uint3 threadID : SV_DispatchThreadID)
{
    RWTexture2D<float2> lutTexture = ResourceDescriptorHeap[renderResources.lutTextureIndex];
    
    float textureWidth, textureHeight;
    lutTexture.GetDimensions(textureWidth, textureHeight);
    
    float cosLO = threadID.x / textureWidth;
    float roughness = threadID.y / textureHeight;
    
    cosLO = max(cosLO, 0.001f);
    
    float3 LO = float3(sqrt(1.0f - cosLO * cosLO), 0.0f, cosLO);
    
    float DFG1 = 0.0f;
    float DFG2 = 0.0f;

    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        float2 u = SampleHammersleySequence(i);

		// Sample directly in tangent/shading space since we don't care about reference frame as long as it's consistent.
        float3 Lh = SampleGGX(u.x, u.y, roughness);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
        float3 Li = 2.0 * dot(LO, Lh) * Lh - LO;

        float cosLi = Li.z;
        float cosLh = Lh.z;
        float cosLoLh = max(dot(LO, Lh), 0.0);

        if (cosLi > 0.0)
        {
            float G = SchlickGGX(cosLi, cosLO, roughness);
            float Gv = G * cosLoLh / (cosLh * cosLO);
            float Fc = pow(1.0 - cosLoLh, 5);

            DFG1 += (1 - Fc) * Gv;
            DFG2 += Fc * Gv;
        }
    }
    
    lutTexture[threadID.xy] = float2(DFG1, 1.0f - DFG2) * INV_NUM_SAMPLES;

}