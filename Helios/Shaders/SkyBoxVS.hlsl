struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

struct TransformData
{
    matrix modelMatrix;
    matrix inverseModelMatrix;
    matrix projectionViewMatrix;
};

StructuredBuffer<float3> positionBuffer : register(t0, space0);
StructuredBuffer<float3> textureCoordBuffer : register(t1, space0);

ConstantBuffer<TransformData> mvpCBuffer : register(b0, space0);

VSOutput VsMain(uint vertexID : SV_VertexID)
{
    matrix mvpMatrix = mul(mvpCBuffer.projectionViewMatrix, mvpCBuffer.modelMatrix);

    VSOutput output;
    output.position.xyz = mul((float3x3) mvpMatrix, positionBuffer[vertexID]);
    output.position.w = output.position.z;
    
    output.texCoord = textureCoordBuffer[vertexID];
    
    output.texCoord = output.position.xyz;

    return output;
}