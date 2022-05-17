#include "BindlessRS.hlsli"
#include "ConstantBuffers.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

ConstantBuffer<LightRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput input) : SV_Target
{
    ConstantBuffer<LightData> pointLightCBuffer = ResourceDescriptorHeap[renderResource.lightDataCBufferIndex];
    
    return float4(pointLightCBuffer.lightColor[renderResource.lightIndex].xyz, 1.0f);
}