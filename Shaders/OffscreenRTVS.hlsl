#include "OffscreenRTRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

StructuredBuffer<float2> positionBuffer : register(t0, space0);
StructuredBuffer<float2> textureCoordsBuffer : register(t1, space0);

[RootSignature(OffscreenRTRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    output.position = float4(positionBuffer[vertexID].xy, 0.0f, 1.0f);
    output.textureCoord = textureCoordsBuffer[vertexID];

    return output;
}

