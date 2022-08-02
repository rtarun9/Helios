#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

ConstantBuffer<LightRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    uint lightID : LIGHT_INDEX;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput, uint instanceID : SV_InstanceID) : SV_Target
{
    ConstantBuffer<LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResource.lightBufferIndex];
  
    return float4(lightBuffer.lightColor[instance], 1.0f);
}