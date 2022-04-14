// Calculates the sampling direction vector using teh current pixel's coordinates.
// Convertes 1 equirectangular projection texture into 6 cube map textures.
// Main Resource used : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/equirect2cube.hlsl.

#include "BindlessRS.hlsli"

ConstantBuffer<CubeFromEquirectRenderResources> renderResources : register(b0);

static const float PI = 3.14159265359;

[numthreads(32, 32, 1)]
void CsMain(uint3 threadID : SV_DispatchThreadID)
{
    Texture2D inputTexture = ResourceDescriptorHeap[renderResources
    .textureIndex];
    
    RWTexture2DArray<float4> outputCubeTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];
    
    float outputTextureWidth, outputTextureHeight, outputTextureDepth;
    outputCubeTexture.GetDimensions(outputTextureWidth, outputTextureHeight, outputTextureDepth);
    
    float2 pixelCoords = threadID.xy / float2(outputTextureWidth, outputTextureHeight);
    
    // Convert pixelCoords into the range of -1 .. 1 and make sure y goes from top to bottom.
    float2 uv = 2.0f * float2(pixelCoords.x, 1.0f - pixelCoords.y) - float2(1.0f, 1.0f);
    
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
    
    // Convert the cartesian coordinate sampling vector into spherical coordinates.
    float phi = atan2(samplingVector.z, samplingVector.x);
    float theta = acos(samplingVector.y);
    
    float4 result = inputTexture.SampleLevel(linearWrapSampler, float2(phi / (2.0f * PI), theta / PI), 0.0f);
    
    outputCubeTexture[threadID] = result;
}