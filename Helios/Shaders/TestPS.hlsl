struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
};

Texture2D testTexture : register(t0, space1);
Texture2D floorTexture : register(t1, space1);

SamplerState textureSampler : register(s0, space1);

struct LightingData
{
    float4 lightPosition;
    float4 cameraPosition;
};

ConstantBuffer<LightingData> lightCBuffer : register(b0, space1);

float4 PsMain(VSOutput input) : SV_Target
{
    float3 pixelToLightDir = normalize(lightCBuffer.lightPosition.xyz - input.worldSpacePosition.xyz);
    float3 viewDir = normalize(lightCBuffer.cameraPosition.xyz - input.worldSpacePosition.xyz);

    float angle = max(dot(input.normal, pixelToLightDir), 0.0f);

    float3 surfaceColor = testTexture.Sample(textureSampler, input.texCoord).xyz;
    float3 ambientColor = surfaceColor * 0.2f;

    float3 coolColor = float3(0.0f, 0.0f, 0.55f) + 0.25f * surfaceColor;
    float3 warmColor = float3(0.3f, 0.2f, 0.0f) + 0.25f * surfaceColor;

    float3 highLightColor = float3(1.0f, 1.0f, 1.0f) * surfaceColor;

    float3 reflectedDir = (reflect(-pixelToLightDir, input.normal));
    float specular = pow(max(dot(reflectedDir, viewDir), 0.0f), 64);

    float3 shadedColor = specular * highLightColor + (1.0f - specular) * (lerp(angle, warmColor, coolColor)) + ambientColor;

    float4 result = float4(shadedColor, 1.0f);

    return result;
}