// clang-format off
// Converts 1 equi-rectangular texture into 6 cube map textures.
// Main Resource used : https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/equirect2cube.hlsl.

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "Utils.hlsli"

ConstantBuffer<interlop::CubeFromEquirectRenderResources> renderResources : register(b0);

[numthreads(8, 8, 1)] 
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID) 
{
    Texture2D inputTexture = ResourceDescriptorHeap[renderResources.textureIndex];
    RWTexture2DArray<float4> outputCubeTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    float outputTextureWidth, outputTextureHeight, outputTextureDepth;
    outputCubeTexture.GetDimensions(outputTextureWidth, outputTextureHeight, outputTextureDepth);

    // Sample from the input texture bidirectionally.
    const float2 pixelCoords = (dispatchThreadID.xy + 0.5f) / float2(outputTextureWidth, outputTextureHeight);

    const float3 samplingVector = getSamplingVector(pixelCoords, dispatchThreadID);

    // Convert the Cartesian coordinate sampling vector into spherical coordinates.
    float phi = atan2(samplingVector.z, samplingVector.x);
    float theta = acos(samplingVector.y);

    outputCubeTexture[dispatchThreadID] = inputTexture.SampleLevel(linearWrapSampler, float2(phi / TWO_PI, theta / PI), 0.0f);
}