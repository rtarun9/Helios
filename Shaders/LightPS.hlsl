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
    ConstantBuffer<PointLightData> pointLightCBuffer = ResourceDescriptorHeap[renderResource.pointLightCBufferIndex];
    
    return float4(pointLightCBuffer.lightColor[renderResource.pointLightIndex].xyz, 1.0f);
}