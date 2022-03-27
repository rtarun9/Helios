struct VSOutput
{
    float4 position : SV_Position;
    float3 texCoord : TEXCOORD;
};

TextureCube cubeMap : register(t0, space1);
SamplerState clampLinearSampler : register(s0, space1);

float4 PsMain(VSOutput input) : SV_Target
{
    return cubeMap.Sample(clampLinearSampler, normalize(input.texCoord));
}