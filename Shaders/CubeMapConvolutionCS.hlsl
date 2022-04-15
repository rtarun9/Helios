#include "BindlessRS.hlsli"

ConstantBuffer<CubeMapConvolutionRenderResources> renderResources : register(b0);

static const float PI = 3.14159265359;

[numthreads(16, 16, 1)]
void CsMain(uint3 threadID : SV_DispatchThreadID)
{
    TextureCube<float4> textureCubeMap = ResourceDescriptorHeap[renderResources
    .textureCubeMapIndex];
    
    RWTexture2DArray<float4> outputIrradianceMap= ResourceDescriptorHeap[renderResources.outputIrradianceMapIndex];
    
    float textureWidth, textureHeight, textureDepth;
    outputIrradianceMap.GetDimensions(textureWidth, textureHeight, textureDepth);
    
    float2 uv = (threadID.xy + float2(0.5f, 0.5f)) / textureWidth;
    uv = uv * float2(2.0f, 2.0f) - float2(1.0f, 1.0f); 
    uv.y *= -1.0f;
    
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
   
    // Sampling happens using spherical coordinates.
    float3 forward = samplingVector;
    float3 up = float3(0.0f, 0.0f, 1.0f);
    float3 left = normalize(cross(up, forward));
    up = normalize(cross(forward, left));
    
    static const float INTEGRATION_STEP_COUNT = 125.0f;
    static const float SAMPLES = INTEGRATION_STEP_COUNT * INTEGRATION_STEP_COUNT;
    
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    
    for (float i = 0.0f; i < INTEGRATION_STEP_COUNT; i++)
    {
        float theta = 2.0f * PI * (i / (INTEGRATION_STEP_COUNT - 1.0f));
        for (float j = 0.0f; j < INTEGRATION_STEP_COUNT; j++)
        {
            float phi = 0.5f * PI * (j / (INTEGRATION_STEP_COUNT - 1.0f));
            float3 tangentSpaceCoordinates = normalize(float3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)));
            float3 worldSpaceCoordinates = tangentSpaceCoordinates.x * left + tangentSpaceCoordinates.y * up + tangentSpaceCoordinates.z * forward;
            float3 irradianceValue = textureCubeMap.SampleLevel(linearWrapSampler, worldSpaceCoordinates, 0.0f).xyz;
            
            irradiance += irradianceValue * cos(phi) * sin(phi);
        }
    }
    
    outputIrradianceMap[threadID] = float4(PI * irradiance / SAMPLES, 0.0f);

}