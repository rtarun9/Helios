// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : Texture_Coord;
};

ConstantBuffer<interlop::FullScreenTrianglePassRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f),
                                               float3(-1.0f, -3.0f, 0.0f)};

    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[clamp(0, 3, vertexID)], 1.0f);
    output.textureCoord = output.position.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return output;
}

float4 PsMain(VSOutput input) : SV_Target
{
    Texture2D<float4> renderTexture = ResourceDescriptorHeap[renderResources.renderTextureIndex];
    float3 color = renderTexture.Sample(linearWrapSampler, input.textureCoord).xyz;

    return float4(color, 1.0f);
}