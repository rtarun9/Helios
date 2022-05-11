#include "BindlessRS.hlsli"
#include "Utils.hlsli"

ConstantBuffer<CubeMapConvolutionRenderResources> renderResources : register(b0);

static const float SAMPLES = 12500.0f;
static const float INV_SAMPLES = 1.0f / SAMPLES;

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
    return float2((float) i * INV_SAMPLES, VanDerCorputRadicalInverse(i));
}

float3 TangentToWorldCoords(float3 v, float3 n, float3 s, float3 t)
{
    return s * v.x + t * v.y + n * v.z;
}

// Source used : https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
float3 SampleHemisphere(float2 u)
{
    float z = 1 - 2.0f * u.x;
    float r = pow(max(0.0f, 1.0f - z * z), 0.5f);
    float phi = 2.0f * PI * u.y;
    return float3(r * cos(phi), r * sin(phi), z);
}

[numthreads(32, 32, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    TextureCube<float4> textureCubeMap = ResourceDescriptorHeap[renderResources.textureCubeMapIndex];
        
    RWTexture2DArray<float4> outputIrradianceMap= ResourceDescriptorHeap[renderResources.outputIrradianceMapIndex];
    
    float textureWidth, textureHeight, textureDepth;
    outputIrradianceMap.GetDimensions(textureWidth, textureHeight, textureDepth);
    
    float2 pixelCoords = dispatchThreadID.xy / float2(textureWidth, textureHeight);
    float3 samplingVector = GetSamplingVector(pixelCoords, dispatchThreadID);
   
    // Calculation of basis vectors for converting a vector from Shading / Tangent space to world space.
    float3 n = samplingVector;
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);
    
    GenerateBasisFromVector(n, t, s);
    
    // Using Monte Carlo integration to find irradiance of the hemisphere.
    // The final result should be 1 / N * summation (f(x) / PDF). PDF for sampling uniformly from hemisphere is 1 / TWO_PI. Since the integral is already consisting of division by 2.0f * PI,
    // There is no need to do this while calculating total irradiance (kd * c * irradiance) is the diffuse IBL. 
    // Resource used for Montecarlo integration + Uniform sampling from hemisphere : https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < SAMPLES; i++)
    {
        float2 u = SampleHammersleySequence(i);
        float3 li = TangentToWorldCoords(SampleHemisphere(u), n, s,t);
        
        float cosTheta = saturate(dot(li, n));
        irradiance += textureCubeMap.SampleLevel(linearWrapSampler, li, 0).rgb * cosTheta;
    }

    // As PDF is 2.0f * PI, I am multiplying by 2.0f so that this multiplication is not required while calculation final diffuse IBL.
    float4 result = float4(irradiance * 2.0f / SAMPLES, 0.0f); 
    outputIrradianceMap[dispatchThreadID] = result;

}