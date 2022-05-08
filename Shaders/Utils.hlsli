#ifndef __UTILS_HLSLI__
#define __UTILS_HLSLI__

static const float MIN_FLOAT_VALUE = 0.0001f;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;

// Base reflectivity of surface.
static const float3 BASE_REFLECTIVITY = float3(0.04f, 0.04f, 0.04f);

float3 GetSamplingVector(float2 pixelCoords, uint3 dispatchThreadID)
{
     // Convert pixelCoords into the range of -1 .. 1 and make sure y goes from top to bottom.
    float2 uv = 2.0f * float2(pixelCoords.x, 1.0f - pixelCoords.y) - float2(1.0f, 1.0f);
    
    float3 samplingVector = float3(0.0f, 0.0f, 0.0f);
    
    // Based on cube face 'index', choose a vector.
    switch (dispatchThreadID.z)
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
    
    return samplingVector;
}
#endif