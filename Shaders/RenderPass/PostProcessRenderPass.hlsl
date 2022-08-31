#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

ConstantBuffer<PostProcessRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float2> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordsBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];

    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xy, 0.0f, 1.0f);
    output.textureCoord = textureCoordsBuffer[vertexID];

    return output;
}

static const float GAMMA_CORRECTION = 0.454545455f;

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    Texture2D<float4> rtvTexture = ResourceDescriptorHeap[NonUniformResourceIndex(renderResource.finalRenderTextureIndex)];
    Texture2D<float3> bloomTexture = ResourceDescriptorHeap[NonUniformResourceIndex(renderResource.bloomTextureIndex)];
    
    ConstantBuffer<PostProcessBuffer> postProcessBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(renderResource.postProcessBufferIndex)];

    float exposure = postProcessBuffer.exposure;

    float4 color = rtvTexture.SampleLevel(pointWrapSampler, input.textureCoord, 0u);
    float3 bloomColor = bloomTexture.SampleLevel(pointWrapSampler, input.textureCoord, 0u);

    // Exposure Tone mapping
    color.rgb = float3(1.0f, 1.0f, 1.0f) - exp(-color.rgb * exposure);
    bloomColor.rgb = float3(1.0f, 1.0f, 1.0f) - exp(-bloomColor.rgb * exposure);

    // Gamma correction.
    color.rgb = pow(color.rgb + bloomColor.rgb, GAMMA_CORRECTION);

    return color;
}