#include "../Common/BindlessRS.hlsli"
#include "../Utils.hlsli"

ConstantBuffer<MeshViewerRenderResources> renderResource : register(b0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
};

[RootSignature(BindlessRootSignature)]
float4 PsMain(VSOutput psInput) : SV_Target
{
    return GetAlbedo(psInput.textureCoord, renderResource.albedoTextureIndex, renderResource.albedoTextureSamplerIndex);    
}