#include "LightRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix projectionViewMatrix;
};

StructuredBuffer<float3> positionBuffer : register(t0, space0);
StructuredBuffer<float2> textureCoordsBuffer : register(t1, space0);
StructuredBuffer<float3> normalBuffer : register(t2, space0);

ConstantBuffer<TransformData> mvpCBuffer : register(b0, space0);

[RootSignature(LightRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    matrix mvpMatrix = mul(mvpCBuffer.projectionViewMatrix, mvpCBuffer.modelMatrix);

    VSOutput output;
    output.position = mul(mvpMatrix, float4(positionBuffer[vertexID], 1.0f));
    output.texCoord = textureCoordsBuffer[vertexID];

    return output;
}