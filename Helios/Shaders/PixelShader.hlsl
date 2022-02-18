struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

Texture2D testTexture : register(t0);
SamplerState textureSampler : register(s0);

float4 PsMain(VSOutput input) : SV_Target
{
    return testTexture.Sample(textureSampler, input.texCoord);

}