struct VSOutput
{
    float4 position : SV_POSITION;
    float2 textureCoord : TEXTURE_COORD;
};

Texture2D rtvTexture : register(t0, space1);
SamplerState pointSampler : register(s0, space1);

float4 PsMain(VSOutput input) : SV_Target
{
    return rtvTexture.Sample(pointSampler, input.textureCoord);
}