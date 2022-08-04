// Calculates the sampling direction vector using the current pixel's coordinates.
// Convertes 1 equirectangular projection texture into 6 cube map textures.
// Main Resource used : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/equirect2cube.hlsl.

#include "Common/BindlessRS.hlsli"
#include "Utils.hlsli"

ConstantBuffer<CubeFromEquirectRenderResources> renderResources : register(b0);

[numthreads(32, 32, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D inputTexture = ResourceDescriptorHeap[renderResources.textureIndex];
    RWTexture2DArray<float4> outputCubeTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    float outputTextureWidth, outputTextureHeight, outputTextureDepth;
    outputCubeTexture.GetDimensions(outputTextureWidth, outputTextureHeight, outputTextureDepth);

    float2 pixelCoords = dispatchThreadID.xy / float2(outputTextureWidth, outputTextureHeight);

    float3 samplingVector = GetSamplingVector(pixelCoords, dispatchThreadID);

    // Convert the cartesian coordinate sampling vector into spherical coordinates.
    float phi = atan2(samplingVector.z, samplingVector.x);
    float theta = acos(samplingVector.y);

    outputCubeTexture[dispatchThreadID] = inputTexture.SampleLevel(linearWrapSampler, float2(phi / TWO_PI, theta / PI), 0.0f);
}