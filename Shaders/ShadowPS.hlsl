#include "BindlessRS.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

[RootSignature(BindlessRootSignature)]
void PsMain(VSOutput input)
{
}   