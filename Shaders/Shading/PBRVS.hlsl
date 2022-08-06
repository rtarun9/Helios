#include "../Common/BindlessRS.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3x3 modelMatrix : MODEL_MATRIX;
    float3 worldSpacePosition : WORLD_SPACE_POSITION;
};

ConstantBuffer<PBRRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)]
VSOutput VsMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResource.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResource.textureBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResource.normalBufferIndex];

    ConstantBuffer<SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];
    ConstantBuffer<TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResource.transformBufferIndex];

    matrix mvpMatrix = mul(mul(transformBuffer.modelMatrix, sceneBuffer.viewMatrix), sceneBuffer.projectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];

    // Normal matrix is the tranpose of the inverse of model matix.
    // necessary when non uniform scaling is used.
    matrix normalMatrix = transpose(transformBuffer.inverseModelMatrix);

    output.normal = normalize(mul(float4(normalBuffer[vertexID], 0.0f), normalMatrix).xyz);
       
    if (renderResource.tangentBufferIndex != -1)
    {
        StructuredBuffer<float4> tangentBuffer = ResourceDescriptorHeap[renderResource.tangentBufferIndex];
        output.tangent = tangentBuffer[vertexID];
    }
    else
    {
        output.tangent = GenerateTangent(output.normal);
    }

    output.modelMatrix = (float3x3)transformBuffer.modelMatrix;
    output.worldSpacePosition = float3(mul(float4(positionBuffer[vertexID], 1.0f), transformBuffer.modelMatrix).xyz);

    return output;
}