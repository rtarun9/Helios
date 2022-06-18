#include "../BindlessRS.hlsli"

[RootSignature(BindlessRootSignature)]
float4 PsMain() : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}