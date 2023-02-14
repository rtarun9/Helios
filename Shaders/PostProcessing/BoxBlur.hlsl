// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : Texture_Coord;
};

ConstantBuffer<interlop::BoxBlurRenderResources> renderResources : register(b0);

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

float PsMain(VSOutput input) : SV_Target
{
    Texture2D<float> tex = ResourceDescriptorHeap[renderResources.textureIndex];
    uint width, height;

    tex.GetDimensions(width, height);
    const float2 pixelSize = float2(1.0f / width, 1.0f / height);

    float sum = 0.0f;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            const float2 offset = float2(float(x), float(y)) * pixelSize;
            sum += tex.Sample(linearClampSampler, input.textureCoord + offset).r;
        }
    }
    
    return sum / 9.0f;
}