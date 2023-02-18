// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::BloomExtractRenderResources> renderResources : register(b0);

// Extract the bright regions from the image.
// Threshold works by comparing luminance for a pixel with the threshold value.
[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)] 
void CsMain(uint3 dispatchThreadID: SV_DispatchThreadID) 
{
	Texture2D<float4> inputTexture = ResourceDescriptorHeap[renderResources.inputTextureIndex];
	RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];
	
	ConstantBuffer<interlop::BloomBuffer> bloomBuffer = ResourceDescriptorHeap[renderResources.bloomBufferIndex];

	float width, height;
	inputTexture.GetDimensions(width, height);

	const float2 texelSize = float2(1.0f / width, 1.0f / height);
	const float2 uvCoords = (dispatchThreadID.xy + 0.5f) * texelSize;

	const float3 color = inputTexture.SampleLevel(linearClampToBorder, uvCoords, 0).xyz;
	if (dot(color, float3(0.2126f, 0.7152f, 0.0722f)) > bloomBuffer.threshHold)
	{
		outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
	}
	else
	{
		outputTexture[dispatchThreadID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}