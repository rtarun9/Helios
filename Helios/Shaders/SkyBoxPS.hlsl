struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

Texture2D cubeMap : register(t0, space1);
SamplerState clampLinearSampler : register(s0, space1);

float4 PsMain(VSOutput input) : SV_Target
{
    return cubeMap.Sample(clampLinearSampler, input.texCoord);
}