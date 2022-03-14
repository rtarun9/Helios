struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

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

ConstantBuffer<TransformData> mvpCBuffer : register(b0, space0);

VSOutput VsMain(VSInput input)
{
    matrix mvpMatrix = mul(mvpCBuffer.projectionViewMatrix, mvpCBuffer.modelMatrix);

    VSOutput output;
    output.position = mul(mvpMatrix, float4(input.position, 1.0f));
    output.texCoord = input.texCoord;

    return output;
}