// clang-format off
// note(rtarun9) : Could just use a texture as the BRDF lut is independent on the actual environment cube map texture,
// but its here just for reference, the math here is *way* to complex for me.

// This compute shader computes the second part of the split sum epic games approximation for the specular part of the cook torrence BRDF.
// The integral we are trying to solve is [integral over hemisphere] (BRDF * (n.wi))dwi.
// We can multiply and divide by the Fresnel factor (which is in the BRDF as well).
// Integral[] = [Integral](BRDF * (n.wi) * Fresnel) / Fresnel) dwi)
// Expanding the Fresnel term in the numerator, and making the term (1 - v.h)^5 as a,
// Integral[] = [Integral]((BRDF / Fresnel) (n.wi) * (F0 + (1 - F0)a)))dwi.
// Integral[] = [Integral]((BRDF / Fresnel) (n.wi) * (F0 + a - a F0)))dwi.
// Integral[] = [Integral]((BRDF / Fresnel) (n.wi) * (F0(1 - a)))dwi + [Integral]((BRDF / Fresnel) (n.wi) * a)dwi.
// The F0 term is constant, can go out of the integral. Also, BRDF involved Fresnel in numerator, so that can be ommited from the BRDF computation.
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

#include "Utils.hlsli"

// References : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/spbrdf.hlsl

ConstantBuffer<interlop::BRDFLutRenderResources> renderResources : register(b0);

static const float NUM_SAMPLES = 2048.0f;
static const float INV_NUM_SAMPLES = 1.0f / NUM_SAMPLES;

// Using the VanDerCorput radical inverse along with HammersleySequence to get the low discrepensy sample i over total
// number of samples (NUM_SAMPLES).
float vanDerCorputRadicalInverse(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}

float2 sampleHammersleySequence(uint i)
{
    return float2((float)i * INV_NUM_SAMPLES,  vanDerCorputRadicalInverse(i));
}

// Get sample vector based on Importance Sampling GGX normal distribution function for a fixed value of roughness.
// The function returns a half vector between Li and Lo.
// Used inorder to orient sample vector towards specular lobe of surface roughness.
float3 importanceSamplingGGX(float2 uv, float roughnessFactor)
{
    const float alpha = pow(roughnessFactor, 2);
    
    const float phi = 2.0f * PI * uv.x;

    const float cosTheta = sqrt((1.0f - uv.y) / (1.0f +  (pow(alpha, 2) - 1.0f) * uv.y));
    const float sinTheta = sqrt(1.0f - pow(cosTheta, 2.0f));
    
    
    // Spherical -> Cartesian coordinates.
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Geometry function : approximates the number / relative surface area of the surface which is actually visible to us.
// If the surface is rough, several microfacets could overshadow and block others, because of which the light reaching
// us will be occluded. Using Smith's method, by changing the angle, we can approximate both self shadowing and geometry
// obstruction. Source :https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf if x
// is viewDirection, then we are calculating geometric obstruction, and if light direction, we are calculating self
// shadowing.
float schlickBeckmannGS(float cosTheta, const float roughnessFactor)
{
    // Why is k = roughness^2 here? From learnopengl:
    // Note that while k takes a as its parameter we didn't square roughness as a as we originally did for other interpretations of a; likely as a is squared here already. I'm not sure whether this is an inconsistency on Epic Games' part or the original Disney paper, but directly translating roughness to a gives the BRDF integration map that is identical to Epic Games' version.
    float k = (roughnessFactor * roughnessFactor) / 2.0f;

    return cosTheta / (max((cosTheta * (1.0f - k) + k), MIN_FLOAT_VALUE));
}

// Smiths method is used for approximation of geometry (both self shadowing and geometry obstruction). (ShlickGGX
// model). Uses SchlickBeckman formula to calculate both geometry obstruction, where the camera cannot see a point as
// some other microfacet is blocking it, or Self shadowing, where the light ray from a point is not able to reach the
// camera.
float smithGeometryFunction(float cosLI, float cosLO, const float roughnessFactor)
{
    return schlickBeckmannGS(cosLI, roughnessFactor) *
           schlickBeckmannGS(cosLO, roughnessFactor);
}

// Calculates the integral fr(p, wi, wo)n.w dwi.
// The horizontal (x axis) stores n.wi, and y axis stores the input roughness value.
// We make assumption that incoming irradiance is 'white' for all directions.
// Integral that is being computed : F0 * integral(fr(p, wi, wo)(1 - (1 - wo.h)^5)n.wdidwi + integral(fr(p, wi, wo) * (1
// - wo.h)^5 n.widwi. This is done to remove the F0 term from the integral (derivation :
// https://learnopengl.com/PBR/IBL/Specular-IBL).
// dispatchThreadID.x -> nDotV, dispatchThreadID.y -> roughness.
// The PBR shader can directly sample the result of the second part of split some integral by the corresponding nDotV and roughness value.
[numthreads(32, 32, 1)] 
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID) 
{
    RWTexture2D<float2> lutTexture = ResourceDescriptorHeap[renderResources.lutTextureIndex];

    float textureWidth, textureHeight;
    lutTexture.GetDimensions(textureWidth, textureHeight);

    float nDotV = (dispatchThreadID.x + 1.0f) / textureWidth;
    float roughness = (dispatchThreadID.y + 1.0f) / textureHeight;

    nDotV  = saturate(nDotV);

    // Calculation of view vector using nDotV (as the LUT is parameterized in terms of nDotV).
    // Assuming that n = (0, 0, 1), and viewDirection is in the XZ plane, viewDirection = (sinLo, 0, cosLo).
    float3 viewDirection = float3(sqrt(1.0f - nDotV * nDotV), 0.0f, nDotV);

    // dfg1 is the integral term involving F0 * integral[brdf . (n.wi) . (1 - (1 - vDotH)^5))dwi]
    // dfg2 is the integral term involving F0 * integral[brdf . (n.wi) . (1 - vDotH)^5 dwi]
    float dfg1 = 0.0f;
    float dfg2 = 0.0f;

    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        // Generate random vector uniformally.
        const float2 uv = sampleHammersleySequence(i);
        
        const float3 halfWayVector = importanceSamplingGGX(uv, roughness);

        // Get incident direction by reflecting view dir around half way vector.
        // The view direction is just the normal.
        const float3 lightDirection = reflect(-viewDirection, halfWayVector);

        // Using the fact that n = (0, 0, 1).
        float nDotL = max(lightDirection.z, 0.0f);
        float nDotH = max(halfWayVector.z, 0.0f);
        float vDotH = saturate(dot(viewDirection, halfWayVector));

        if (nDotL > 0.0)
        {
            // The microfacet BRDF formulation.
            const float g = smithGeometryFunction(nDotL, nDotV, roughness);
            const float gv = g * vDotH / (nDotH * nDotV);
            const float f = pow(1.0 - vDotH, 5);

            dfg1 += (1 - f) * gv;
            dfg2 += f * gv;
        }
    }

    lutTexture[dispatchThreadID.xy] = float2(dfg1, dfg2) * INV_NUM_SAMPLES;
}