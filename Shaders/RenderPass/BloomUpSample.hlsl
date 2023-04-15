// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::BloomUpSampleRenderResources> renderResources : register(b0);

// For upsampling, add the previous mip level with the current blurred level. Do this progressively until mip level 0 is reached.
// The blurring is performed using a 3x3 tent filter (slide 111 of the cod next generation post processing presentation).
[RootSignature(BindlessRootSignature)]
[numthreads(12, 8, 1)] 
void CsMain(uint3 dispatchThreadID: SV_DispatchThreadID) 
{
	Texture2D<float4> inputPreviousUpSampleTexture = ResourceDescriptorHeap[renderResources.inputPreviousUpSampleSrvIndex];
	RWTexture2D<float4> inputCurrentDownSampleTexture = ResourceDescriptorHeap[renderResources.inputCurrentDownSampleUavIndex];
	RWTexture2D<float4> outputCurrentUpSampleTexture = ResourceDescriptorHeap[renderResources.outputCurrentUpSampleMipIndex];

	ConstantBuffer<interlop::BloomBuffer> bloomBuffer = ResourceDescriptorHeap[renderResources.bloomBufferIndex];
	float width, height;
	outputCurrentUpSampleTexture.GetDimensions(width, height);
	const float2 outputTexelSize = 1.0f / float2(width, height);

	const float2 uvCoords = (dispatchThreadID.xy + 0.5f) * outputTexelSize;

	inputPreviousUpSampleTexture.GetDimensions(width, height);
	const float2 inputTexelSize = 1.0f / float2(width, height);


	// A B C    Weights : 1 2 1
	// D E F	Weights : 2 4 2
	// G H I	Weights : 1 2 1

	// We add the current mip level color with the blurred mip level color of the previous mip (slide 112 of the presentation).
	// If mips are from A -> E,
	// A (upsampled) = A + Blur(B)
	// B (upsampled) = B + Blur(C) and so on.

	outputCurrentUpSampleTexture[dispatchThreadID.xy] = inputCurrentDownSampleTexture[dispatchThreadID.xy];

	if (renderResources.bloomPassIndex == interlop::BLOOM_PASSES - 1)
	{
		// We have already set the data of current pixel to that of the current down sample texture, so we can just return.
		return;
	}

	const float3 A = (1.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-1.0f, -1.0f) * inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 B = (2.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(0.0f, -1.0f) *  inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 C = (1.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(1.0f, -1.0f) *  inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 D = (2.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-1.0f, 0.0f) *  inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 E = (4.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(0.0f, 0.0f) *   inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 F = (2.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(1.0f, 0.0f) *   inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 G = (1.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-1.0f, 1.0f) *  inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 H = (2.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(0.0f, 1.0f) *   inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	const float3 I = (1.0f / 16.0f) * inputPreviousUpSampleTexture.SampleLevel(linearClampToBorder, uvCoords + float2(1.0f, 1.0f) *   inputTexelSize * bloomBuffer.radius, renderResources.inputPreviousUpSampleMipLevel).xyz;
	
	outputCurrentUpSampleTexture[dispatchThreadID.xy] +=  float4(A + B + C + D + E + F + G + H + I, 1.0f);
}