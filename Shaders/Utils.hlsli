#ifndef __UTILS_HLSLI__
#define __UTILS_HLSLI__

static const float MIN_FLOAT_VALUE = 0.00001f;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;
static const float INV_PI = 1.0f / PI;
static const float INV_TWO_PI = 1.0f / TWO_PI;

float4 GetAlbedo(float2 texCoord, uint albedoTextureIndex, uint albedoTextureSamplerIndex)
{
    if (albedoTextureIndex == -1)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];
    if (!albedoTextureSamplerIndex)
    {
        return albedoTexture.Sample(linearWrapSampler, texCoord);
    }

    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(albedoTextureSamplerIndex)];
    return albedoTexture.Sample(samplerState, texCoord);
}


float3 GetSamplingVector(float2 pixelCoords, uint3 dispatchThreadID)
{
    // Convert pixelCoords into the range of -1 .. 1 and make sure y goes from top to bottom.
    float2 uv = 2.0f * float2(pixelCoords.x, 1.0f - pixelCoords.y) - float2(1.0f, 1.0f);

    float3 samplingVector = float3(0.0f, 0.0f, 0.0f);

    // Based on cube face 'index', choose a vector.
    switch (dispatchThreadID.z)
    {
    case 0:
        samplingVector = float3(1.0, uv.y, -uv.x);
        break;
    case 1:
        samplingVector = float3(-1.0, uv.y, uv.x);
        break;
    case 2:
        samplingVector = float3(uv.x, 1.0, -uv.y);
        break;
    case 3:
        samplingVector = float3(uv.x, -1.0, uv.y);
        break;
    case 4:
        samplingVector = float3(uv.x, uv.y, 1.0);
        break;
    case 5:
        samplingVector = float3(-uv.x, uv.y, -1.0);
        break;
    }

    samplingVector = normalize(samplingVector);

    return samplingVector;
}

float3 GetNormal(float3 inputNormal, float2 texCoord, float4 tangent, float3x3 modelMatrix, uint normalTextureIndex, uint normalTextureSamplerIndex)
{
    if (normalTextureIndex != -1)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];

        inputNormal = normalize(inputNormal);
        
        float3 normal = float3(0.0f, 0.0f, 0.0f);

        if (!normalTextureSamplerIndex)
        {
            normal = normalize(2.0f * normalTexture.Sample(anisotropicSampler, texCoord).xyz - float3(1.0f, 1.0f, 1.0f));
        }
        else
        {
            SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(normalTextureSamplerIndex)];
            normal = normalize(2.0f * normalTexture.Sample(samplerState, texCoord).xyz - float3(1.0f, 1.0f, 1.0f));
        }

        tangent.xyz = normalize(tangent.xyz);

        float3 bitangent = normalize(cross(inputNormal, tangent.xyz)) * tangent.w;
        float3x3 tbn = float3x3(tangent.xyz, bitangent, inputNormal);

        normal = mul(normal, tbn);
        normal = normalize(mul(normal, modelMatrix));

        return normal;
    }
    else
    {
        return normalize(inputNormal);
    }
}

float4 GenerateTangent(float3 normal)
{
    float3 tangent = cross(normal, float3(0.0f, 1.0f, 0.0));
    tangent = normalize(lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), tangent, step(MIN_FLOAT_VALUE, dot(tangent, tangent))));

    return float4(tangent, 1.0f);
}

#endif