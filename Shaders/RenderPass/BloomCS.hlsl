// This shader will handle prefiltering, upsampling + filtering and downsampling + filtering.
// Inspired by the Call of duty - Advanced warefare presentation, and uses Unity's bloom implementation as a base reference (which also follows the COD presentation), but implemented in
// Helios on compute shaders.
// Unity's implementation : https://github.com/Unity-Technologies/Graphics/blob/master/com.unity.postprocessing/PostProcessing/Shaders/Builtins/Bloom.shader.
// The Cherno's "Hazel Engine's" bloom implementation (this link does not go to the private repo of Hazel, but to a renderer which uses the same bloom shader as hazel) : https://github.com/garlfin/garEnginePublic/blob/4a805b44d62527dc88e45b84e5942ce99ed3420c/gESilk/resources/shader/bloom.glsl.
// The original  COD / AW presentation slides are available here : http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare.
// Reference for karis average impl : https://github.com/adepke/VanguardEngine/blob/a493d33c1a62e606ea67207c128c464db4923a05/VanguardEngine/Shaders/Bloom/Extract.hlsl

#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"

// Based on threshHold vlue, prefilter the input texture.
// Quadratic Threshold, using a curve equation y = (threshHold - knee), knee * 2, 0.25 / knee. Function inspired by Hazel's bloom impl.
float3 QuadraticThreshold(float3 inputColor, float threshHoldValue, float3 curve)
{
    // Pixel component with greatest brightness (Maximum)
    float brightness = max(inputColor.r, max(inputColor.g, inputColor.b));

    // Under-threshold part: quadratic curve
    float rq = clamp(brightness - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve.
    float3 color = inputColor * max(rq, brightness - threshHoldValue) / max(brightness, EPSILON);

    return color;
}

// Curve : (threshHold - knee), knee * 2, 0.25 / knee.
float3 Prefilter(float3 inputColor, float2 textureCoords, float2 threshHoldParams)
{
    const float clampValue = 50.0f;
    inputColor = min(float3(clampValue, clampValue, clampValue), inputColor);
    return QuadraticThreshold(inputColor.xyz, threshHoldParams.x, float3(threshHoldParams.x - threshHoldParams.y, threshHoldParams.y * 2.0f, 0.25f / threshHoldParams.y));
}

// 13 bilinear fetches (36 texel) downsampling.
// Sample layout : 
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
float3 DownSample13Fetches(Texture2D<float3> inputTexture, float2 textureCoords, float2 texelSize)
{
    float3 A = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2(-1.0, -1.0));
    float3 B = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 0.0, -1.0));
    float3 C = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 1.0, -1.0));
    float3 D = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2(-0.5, -0.5));
    float3 E = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 0.5, -0.5));
    float3 F = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2(-1.0,  0.0));
    float3 G = inputTexture.Sample(linearClampSampler, textureCoords                                 );
    float3 H = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 1.0,  0.0));
    float3 I = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2(-0.5,  0.5));
    float3 J = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 0.5,  0.5));
    float3 K = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2(-1.0,  1.0));
    float3 L = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 0.0,  1.0));
    float3 M = inputTexture.Sample(linearClampSampler, textureCoords + texelSize * float2( 1.0,  1.0));

    // The innerbox (i.e DEIJ) have a weight of 0.5 each (divided by 4 for averaging).
    // The remaining 4 outer boxes (A B G F || B C H G || F G L K || G H M L) have weightage of 0.125 each (/4 for averaging).
    float averagedWeightInner = (0.25f) * 0.5f;
    float averagedWeightOuter = (0.25f) * 0.125f;

    float3 output = (D + E + I + J) * averagedWeightInner;
    output += (A + B + G + F) * averagedWeightOuter;
    output += (B + C + H + G) * averagedWeightOuter;
    output += (F + G + L + K) * averagedWeightOuter;
    output += (G + H + M + L) * averagedWeightOuter;

    return output;
}

// 9-tap bilinear upsampler (tent filter)
float3 UpsampleTent(Texture2D<float3> inputTexture, float2 textureCoords, float2 texelSize)
{
    float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0);

    float3 result = float3(0.0f, 0.0f, 0.0f);
    result =  inputTexture.SampleLevel(linearClampSampler, textureCoords - d.xy, 0u);
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords - d.wy, 0u) * 2.0f;
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords - d.zy, 0u); 
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords + d.zw, 0u) * 2.0f;
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords       , 0u) * 4.0f;
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords + d.xw, 0u) * 2.0f;
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords + d.zy, 0u);
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords + d.wy, 0u) * 2.0f;
    result += inputTexture.SampleLevel(linearClampSampler, textureCoords + d.xy, 0u);

    return  result * (1.0 / 16.0);
}

// Required for karis average.
float LinearToLuminanceColor(float3 linearInputColor)
{
	return dot(linearInputColor, float3(0.2126f, 0.7152f, 0.0722f));  
}

float3 KarisAverage(Texture2D<float3> inputTexture, float2 textureCoords, float2 texelSize)
{
	float3 a = inputTexture.Sample(linearClampSampler, (textureCoords + float2(-1.0f, -1.0f)) * texelSize).rgb;
	float3 b = inputTexture.Sample(linearClampSampler, (textureCoords + float2(1.0f, -1.0f)) * texelSize).rgb; 
	float3 c = inputTexture.Sample(linearClampSampler, (textureCoords + float2(-1.0f, 1.0f)) * texelSize).rgb; 
	float3 d = inputTexture.Sample(linearClampSampler, (textureCoords + float2(1.0f, 1.0f)) * texelSize).rgb;  

	float3 weightA = 1.0f / (1.0f + LinearToLuminanceColor(a));
	float3 weightB = 1.0f / (1.0f + LinearToLuminanceColor(b));
	float3 weightC = 1.0f / (1.0f + LinearToLuminanceColor(c));
	float3 weightD = 1.0f / (1.0f + LinearToLuminanceColor(d));
	
	return (a * weightA + b * weightB + c * weightC + d * weightD) / (weightA + weightB + weightC + weightD);
}

float3 GaussianBlurHorizontal(Texture2D<float3> inputTexture, float2 textureCoords, float2 texelSize)
{
    const int WEIGHTS = 5;

    const float weight[WEIGHTS] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};

    float3 result = inputTexture.Sample(linearClampSampler, textureCoords) * weight[0];
    for(int i = 1; i < WEIGHTS; ++i)
    {
        result += inputTexture.Sample(linearClampSampler, textureCoords + float2(texelSize.x * i, 0.0)).rgb * weight[i];
        result += inputTexture.Sample(linearClampSampler, textureCoords - float2(texelSize.x * i, 0.0)).rgb * weight[i];
    }

    return result;
}

float3 GaussianBlurVertical(Texture2D<float3> inputTexture, float2 textureCoords, float2 texelSize)
{
    const int WEIGHTS = 5;

    const float weight[WEIGHTS] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};

    float3 result = inputTexture.Sample(linearClampSampler, textureCoords) * weight[0];
    for(int i = 1; i < WEIGHTS; ++i)
    {
        result += inputTexture.Sample(linearClampSampler, textureCoords + float2(0.0f, texelSize.y * i)).rgb * weight[i];
        result += inputTexture.Sample(linearClampSampler, textureCoords - float2(0.0f, texelSize.y * i)).rgb * weight[i];
    }

    return result;
}

ConstantBuffer<BloomPassRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float3> inputTexture = ResourceDescriptorHeap[renderResources.inputTextureIndex];
    RWTexture2D<float3> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    ConstantBuffer<BloomBuffer> bloomBuffer = ResourceDescriptorHeap[renderResources.bloomBufferIndex];

    float2 texelSize = bloomBuffer.texelSize;
    float2 textureCoords = float2(dispatchThreadID.xy) * float2(texelSize);

    float3 inputTextureColor = inputTexture.Sample(linearClampSampler, textureCoords);

    if (bloomBuffer.shaderUsage == BloomShaderUsage::PreFilter)
    {
        float3 downSampledResult = DownSample13Fetches(inputTexture, textureCoords, texelSize);
        float3 color = Prefilter(downSampledResult, textureCoords, bloomBuffer.threshHoldParams);
        outputTexture[dispatchThreadID.xy] =  color;
    }
    else if (bloomBuffer.shaderUsage == BloomShaderUsage::FirstDownsample)
    {
        float2 textureInputDimensions;
	    inputTexture.GetDimensions(textureInputDimensions.x, textureInputDimensions.y);
	
	    float2 texelSize = 1.0f / textureInputDimensions;
	    float2 texelCenter = (dispatchThreadID.xy * 2.0f + float2(1.0f, 1.0f) * texelSize);

        outputTexture[dispatchThreadID.xy] = KarisAverage(inputTexture, texelCenter, texelSize);
    }

    else if (bloomBuffer.shaderUsage == BloomShaderUsage::Downsample)
    {
        float3 downSampledResult =  DownSample13Fetches(inputTexture, textureCoords, texelSize);
        outputTexture[dispatchThreadID.xy] = downSampledResult;
    }
    else if (bloomBuffer.shaderUsage == BloomShaderUsage::Upsample)
    {
        float3 upsampleResult = UpsampleTent(inputTexture, textureCoords, texelSize);
        outputTexture[dispatchThreadID.xy] = upsampleResult;
    }
    else if (bloomBuffer.shaderUsage == BloomShaderUsage::GaussianBlurVertical)
    {
        outputTexture[dispatchThreadID.xy] = GaussianBlurVertical(inputTexture, textureCoords, texelSize);
    }
    else if (bloomBuffer.shaderUsage == BloomShaderUsage::GaussianBlurHorizontal)
    {
        outputTexture[dispatchThreadID.xy] = GaussianBlurHorizontal(inputTexture, textureCoords, texelSize);
    }
}