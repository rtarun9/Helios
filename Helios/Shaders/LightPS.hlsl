struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

float4 PsMain(VSOutput input) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}