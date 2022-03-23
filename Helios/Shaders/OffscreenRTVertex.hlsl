struct VSInput
{
    float2 position : POSITION;
    float2 textureCoord : TEXTURE_COORD;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

VSOutput VsMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.textureCoord = input.textureCoord;

    return output;
}

