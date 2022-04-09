#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};


[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    return float4(lightColor, 1.0f);
}