struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

struct TransformData
{
    matrix MVP;
};


ConstantBuffer<TransformData> mvpCBuffer : register(b0, space0);

VSOutput VsMain(VSInput input)
{
    VSOutput output;
    output.position = mul(mvpCBuffer.MVP, float4(input.position, 1.0f));
    output.texCoord = input.texCoord;

    return output;
}