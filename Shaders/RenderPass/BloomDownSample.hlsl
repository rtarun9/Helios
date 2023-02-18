// clang-format off


#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::BloomDownSampleRenderResources> renderResources : register(b0);

float luminance(const float3 sourceColor)
{
	return 0.299 * sourceColor.r +  0.587 * sourceColor.g +  0.114 * sourceColor.b;
}

// According to the call of duty presentation, performing a partial karis average (i.e a non linear intensity mapping function)
// prevents firefly effect. The Karis average is done only for mip 0 -> 1.
// Source (From Karis himself : https://graphicrants.blogspot.com/2013/12/tone-mapping.html)
float3 karisAverage(const float3 sourceColorA, const float3 sourceColorB, const float3 sourceColorC, const float3 sourceColorD)
{
	const float A = 1.0f / (1.0f + luminance(sourceColorA));
	const float B = 1.0f / (1.0f + luminance(sourceColorB));
	const float C = 1.0f / (1.0f + luminance(sourceColorC));
	const float D = 1.0f / (1.0f + luminance(sourceColorD));

	// Return weighted average of A, B, C and D.
	return (sourceColorA * A + sourceColorB * B + sourceColorC * C + sourceColorD * D) / (A + B + C + D);
}

float3 average(const float3 sourceColorA, const float3 sourceColorB, const float3 sourceColorC, const float3 sourceColorD)
{
	return (sourceColorA + sourceColorB + sourceColorC +  sourceColorD) * 0.25f;
}


[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)] 
void CsMain(uint3 dispatchThreadID: SV_DispatchThreadID) 
{
	Texture2D<float4> inputTexture = ResourceDescriptorHeap[renderResources.inputTextureIndex];
	RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

	const float2 texelSize = renderResources.texelSize;

	const float2 uvCoords = (dispatchThreadID.xy + 0.5f) * texelSize;

	// Call of duty uses a 13 bilinear fetch for the down sampling pass.
	// Slide to refer to : page 103 in the Next Generation Post Processing presentation.
	// The samples are taken in form of 4 square, where each square has one pixel overlapping with the uvCoords (This results in 
	// 8 samples, plus one at uvCoords, plus 4 additional samples at center of said squares).
	// Visual representation:
	// A   B   C
	//   D   E
	// F   G   H
	//   I   J
	// K   L   M
	// Weight for each of the samples:
	// D E I J -> 0.5
	// Quad 1 : A B F G, -> 0.125
	// Quad 2 : B G C H  -> 0.125
	// Quad 3 : F G K L  -> 0.125
	// Quad 4 : G H L M  -> 0.125

	const float3 A = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-2.0f, -2.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 B = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 0.0f, -2.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 C = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 2.0f, -2.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 D = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-1.0f, -1.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 E = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 1.0f, -1.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 F = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-2.0f,  0.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 G = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 0.0f,  0.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 H = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 2.0f,  0.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 I = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-1.0f,  1.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 J = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 1.0f,  1.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 K = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2(-2.0f,  2.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 L = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 0.0f,  2.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;
	const float3 M = inputTexture.SampleLevel(linearClampToBorder, uvCoords + float2( 2.0f,  2.0f) * texelSize, renderResources.inputTextureMipLevel).xyz;

	if (renderResources.bloomPassIndex == 0)
	{
		outputTexture[dispatchThreadID.xy] = float4(karisAverage(D, E, I, J), 1.0f);
		return;
	}


	const float3 centerQuad =     0.5f   * average(D, E, I, J);
	const float3 upperLeftQuad =  0.125f * average(A, B, F, G);
	const float3 upperRightQuad = 0.125f * average(B, G, C, H);
	const float3 lowerLeftQuad =  0.125f * average(F, G, K, L);	
	const float3 lowerRightQuad = 0.125f * average(G, L, H, M);

	outputTexture[dispatchThreadID.xy] = float4(centerQuad + upperLeftQuad + upperRightQuad + lowerLeftQuad + lowerRightQuad, 1.0f);
}