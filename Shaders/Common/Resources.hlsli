#ifndef __RESOURCES_HLSLI__
#define __RESOURCES_HLSLI__

struct TextureHandle
{
	uint srvIndex;
	uint uavIndex;
	uint samplerIndex;
};

template <typename T> 
T Load2D(TextureHandle textureHandle, uint2 textureCoords)
{
	Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureHandle.srvIndex)];

	return texture.Load(uint3(textureCoords, 0));
}

template <typename T> 
T Load3D(TextureHandle textureHandle, uint3 textureCoords)
{
	Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureHandle.srvIndex)];

	return texture.Load(uint4(textureCoords, 0));
}

template <typename T> 
T Sample2D(TextureHandle textureHandle, float2 textureCoords)
{
	Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureHandle.srvIndex)];

	if (textureHandle.samplerIndex == -1)
	{
		return texture.Sample(anisotropicSampler, textureCoords);
	}


	SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(textureHandle.samplerIndex)];
	return texture.Sample(samplerState, textureCoords);	
}

template <typename T> 
T Sample3D(TextureHandle textureHandle, float3 textureCoords)
{
	Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureHandle.srvIndex)];

	if (textureHandle.samplerIndex == -1)
	{
		return texture.Sample(anisotropicSampler, textureCoords);
	}


	SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(textureHandle.samplerIndex)];
	return texture.Sample(samplerState, textureCoords);
}

#endif