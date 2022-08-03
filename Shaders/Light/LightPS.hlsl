#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

ConstantBuffer<LightRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    return psInput.color;
}