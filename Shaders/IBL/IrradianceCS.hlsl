#include "../Common/BindlessRS.hlsli"
#include "../Common/Utils.hlsli"

ConstantBuffer<IrradianceRenderResources> renderResources : register(b0);

static const uint SAMPLES = 65536;
static const float INV_SAMPLES = 1.0f / (float)SAMPLES;

// Reference : https://learnopengl.com/PBR/IBL/Diffuse-irradiance

// Why this shader exists :
// The diffuse half of the reflectance equation is integral(over hemisphere at point p) kD * albedo / PI * li(p, wi) * n.wi
// Now, taking the constant terms out, the integral because (constant terms) * integarl (over hemisphere at point p) li(p, wi) * n.wi.
// For speed, we precompute this integral into a diffuse irradiance cube map, so that our PBR shader can use a vector wi, sample into cube map and get the diffuse radiance at that point.
// Every sample direction in the cube map will store the radiance by taking sample directions over the hemisphere at point p.
// Note that in this case wi is none other than the normal vector of a particular surface's point (p).
// The necessary is attained by convoluting the cube map : i.e calculating the average radiance of each direction wi in the hemisphere at point p, oriented along the surface normal.

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

// Get the i'th point from Hammersley sequence.
float2 SampleHammersleySequence(uint i)
{
    return float2((float) i * INV_SAMPLES, VanDerCorputRadicalInverse(i));
}

// Source used : https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
// Two uniform random numbers are provided in u, and a vector on the hemisphere is returned.
float3 UniformSampleHemisphere(float2 u)
{
    float z = 1 - 2.0f * u.x;
    float r = pow(max(0.0f, 1.0f - z * z), 0.5f);
    float phi = 2.0f * PI * u.y;
    return float3(r * cos(phi), r * sin(phi), z);
}

float3 TangentToWorldCoords(float3 v, float3 n, float3 s, float3 t)
{
    return s * v.x + t * v.y + n * v.z;
}

// Calculates the variable diffuse parts of the diffuse brdf (integral n.wi * li(p, wi)).
[numthreads(32, 32, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    TextureCube<float4> cubeMapTexture = ResourceDescriptorHeap[renderResources.skyBoxTextureIndex];
        
    RWTexture2DArray<float4> outputIrradianceMap = ResourceDescriptorHeap[renderResources.ouputIrradianceMapIndex];
    
    float textureWidth, textureHeight, textureDepth;
    outputIrradianceMap.GetDimensions(textureWidth, textureHeight, textureDepth);
    
    // For each of the 6 cube faces, there will be threads which have the same 2d pixel coord. Do the sampling bilinearly.
    float2 pixelCoords = (0.5f + dispatchThreadID.xy) / textureWidth;
  
    // Based on current texture pixel coord (and the dispatch's z parameter : or the number of groupz on the z axis (hardcoded to 6), it will calculate the normalized samling direction which we use to sample into the cube map.
    // Reference : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/irmap.hlsl
    float3 samplingVector = GetSamplingVector(pixelCoords, dispatchThreadID);
   
    // Calculation of basis vectors for converting a vector from Shading / Tangent space to world space.
    float3 normal = samplingVector;
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);
    
    ComputeBasisVectors(normal, s, t);
    
    // Using Monte Carlo integration to find irradiance of the hemisphere.
    // The final result should be 1 / N * summation (f(x) / PDF). PDF for sampling uniformly from hemisphere is 1 / TWO_PI. Since the integral is already consisting of division by 2.0f * PI,
    // There is no need to do this while calculating total irradiance (kd * c * irradiance) is the diffuse IBL. For convinience sake, the result of this CS is multiplied back by 2.0. Division by PI already happens during BRDF calculation.
    // Resource used for Montecarlo integration + Uniform sampling from hemisphere : https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration, https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    
    // This loop, will for each sampling direction (i.e the surface normal) will generate random points on the hemisphere at p, get its world space coordinate such that it is centered at the particular pixel / point, 
    // and convolute the input cube map to average the radiance, so as to get the resultant irradiance from a particular wi direction.
    for (uint i = 0; i < SAMPLES; i++)
    {
        // Get a uniform random variable which will be used to get a point on the hemisphere.
        float2 u = SampleHammersleySequence(i);

        // li is the incoming light direction vector in world space.
        // The particular point on the hemisphere is chosen randomly using the Hammersley sequence.
        // This random point on the hemisphere is converted to world space centered at origin (at the pixel coord of the cube face), and we can use it to sample into the input cube map, thereby finally calculating the average radiance of hemisphere centered at pixelCoords (For each face). 
        float3 li = TangentToWorldCoords(UniformSampleHemisphere(u), normal, s, t);
        
        float cosTheta = saturate(dot(li, normal));
        irradiance += cubeMapTexture.Sample(linearClampSampler, li).rgb * cosTheta;
    }

    // As PDF is 2.0f * PI, I am multiplying by 2.0f so that this multiplication is not required while calculation final diffuse IBL.
    float4 result = float4(irradiance * 2.0f / SAMPLES, 1.0f); 
    outputIrradianceMap[dispatchThreadID] = result;
}