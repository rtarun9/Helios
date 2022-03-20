struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

static const float GAMMA_CORRECTION = 0.454545455f;

float4 PsMain(VSOutput input) : SV_Target
{
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    return float4(pow(lightColor, GAMMA_CORRECTION), 1.0f);
}