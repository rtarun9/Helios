struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 worldSpacePosition : WORLD_SPACE_POSITION;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix projectionViewMatrix;
};

ConstantBuffer<TransformData> mvpCBuffer : register(b0, space0);

VSOutput VsMain(VSInput input)
{
    matrix mvpMatrix = mul(mvpCBuffer.projectionViewMatrix, mvpCBuffer.modelMatrix);

    VSOutput output;
    output.position = mul(mvpMatrix, float4(input.position, 1.0f));
    output.texCoord = input.texCoord;
    output.normal = mul((float3x3) (transpose(mvpCBuffer.inverseModelMatrix)), input.normal);
    output.normal = normalize(output.normal);
    output.worldSpacePosition = mul(mvpCBuffer.modelMatrix, float4(input.position, 1.0f));

    return output;
}