#include "BindlessRS.hlsli"
#include "Utils.hlsli"

// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spmap.hlsl.

ConstantBuffer<BRDFConvolutionRenderResources> renderResources : register(b0);

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
// Used inorder to orient sample vector towards specular lobe of surface roughness.
float3 SampleGGX(float2 u, float roughness)
{
    float alpha = pow(roughness, 2);
    
    float cosTheta = sqrt((1.0f - u.y) / (1.0f + (pow(alpha, 2) - 1.0f) * u.y));
    float sinTheta = sqrt(1.0f - pow(cosTheta, 2.0f));
    
    float phi = PI * 2.0f * u.x;
    
    // The Spherical coordinate should be converted to cartesian coordinates before returning.
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}


float SchlickGS(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0f - k) + k);
}

// SchlickGGX approximation of geometric attenuation (using Smith's method).
float SchlickGGX(float cosLI, float cosLO, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0f;
    
    return SchlickGS(cosLI, k) * SchlickGS(cosLO, k);
}

// Calculates the integral fr(p, wi, wo)n.w dwi.
// The horizontal (x axis) stores n.wi, and y axis stores the input roughness value.
// We make assumption that incoming irradiance is 'white' for all directions.
// Integral that is being computed : F0 * integral(fr(p, wi, wo)(1 - (1 - wo.h)^5)n.wdidwi + integral(fr(p, wi, wo) * (1 - wo.h)^5 n.widwi.
// This is done to remove the F0 term from the integral (derivation : https://learnopengl.com/PBR/IBL/Specular-IBL).
[numthreads(32, 32, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float2> lutTexture = ResourceDescriptorHeap[renderResources.lutTextureIndex];
    
    float textureWidth, textureHeight;
    lutTexture.GetDimensions(textureWidth, textureHeight);
    
    float cosLO = (dispatchThreadID.x + 1.0f) / textureWidth;
    float roughness = (dispatchThreadID.y + 1.0f) / textureHeight;
    
    cosLO = saturate(cosLO);
    
    // Calculation of view vector using nDotV (as the LUT is parameterized in terms of nDotV).
    // Assuming that n = (0, 0, 1), and viewDirection is in the XZ plane, viewDirection = (sinLo, 0, cosLo).
    float3 lo = float3(sqrt(1.0f - cosLO * cosLO), 0.0f, cosLO);
    
    float dfg1 = 0.0f;
    float dfg2 = 0.0f;

    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        float2 u = SampleHammersleySequence(i);

		// Sample directly in tangent/shading space since we don't care about reference frame as long as it's consistent.
        float3 lh = SampleGGX(u, roughness);

		// Compute incident direction (li) by reflecting viewing direction (lo) around half-vector (lh).
        float3 li = 2.0 * dot(lo, lh) * lh - lo;

        // Using the fact that n = (0, 0, 1) and x.n = x.z.
        float cosLi = max(li.z, 0.0f);
        float cosLh = max(lh.z, 0.0f);
        float cosLoLh = saturate(dot(lo, lh));

        if (cosLi > 0.0)
        {
            // The microfacet BRDF formulation.
            float g = SchlickGGX(cosLi, cosLO, roughness);
            float gv = g * cosLoLh / (cosLh * cosLO);
            float fc = pow(1.0 - cosLoLh, 5);

            dfg1 += (1 - fc) * gv;
            dfg2 += fc * gv;
        }
    }
    
    lutTexture[dispatchThreadID.xy] = float2(dfg1, dfg2) * INV_NUM_SAMPLES;
}