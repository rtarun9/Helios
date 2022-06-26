#include "../Common/BindlessRS.hlsli"

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    return float4(psInput.color);
}