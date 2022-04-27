#include "BindlessRS.hlsli"

ConstantBuffer<CubeMapConvolutionRenderResources> renderResources : register(b0);

static const float PI = 3.14159265359;
static const float MIN_FLOAT_VALUE = 0.00001f;

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

// Currently not able to find a resource that explains this.
// TODO : Find out the math behind this (source : https://github.com/Nadrin/PBR/blob/cd61a5d59baa15413c7b0aff4a7da5ed9cc57f61/data/shaders/hlsl/irmap.hlsl#L41). 
float3 SampleHemisphere(float2 u)
{
    float u1P = sqrt(max(0.0f, 1.0f - u.x * u.x));
    return float3(cos(2.0f * PI * u.y) * u1P, sin(2.0f * PI * u.y) * u1P, u.x);
}

[numthreads(32, 32, 1)]
void CsMain(uint3 threadID : SV_DispatchThreadID)
{
    TextureCube<float4> textureCubeMap = ResourceDescriptorHeap[renderResources
    .textureCubeMapIndex];
    
    RWTexture2DArray<float4> outputIrradianceMap= ResourceDescriptorHeap[renderResources.outputIrradianceMapIndex];
    
    float textureWidth, textureHeight, textureDepth;
    outputIrradianceMap.GetDimensions(textureWidth, textureHeight, textureDepth);
    
    float2 uv = threadID.xy / float2(textureWidth, textureHeight);
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
   
    // Calculation of basis vectors for converting a vector from Shading / Tangent space to world space.
    float3 N = samplingVector;
    float3 T = T = cross(N, float3(0.0, 1.0, 0.0));
    T = normalize(lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(MIN_FLOAT_VALUE, dot(T, T))));
    float3 S = normalize(cross(N, T));
    
    // Using Monte Carlo integration to find irradiance of the hemisphere.
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < SAMPLES; i++)
    {
        float2 u = SampleHammersleySequence(i);
        float3 Li = TangentToWorldCoords(SampleHemisphere(u), N, S, T);
        
        float cosTheta = max(dot(Li, N), 0.0f);
        irradiance += 2.0f * textureCubeMap.SampleLevel(linearWrapSampler, Li, 0).rgb * cosTheta;
    }

    float4 result = float4(irradiance / SAMPLES, 0.0f); 
    outputIrradianceMap[threadID] = result;

}