#include "Scene/Lights.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

using namespace math;

namespace helios::scene
{
    Lights::Lights(const gfx::GraphicsDevice* const graphicsDevice)
    {
        // Create light related constant buffers.
        m_lightsBuffer = graphicsDevice->createBuffer<interlop::LightBuffer>(gfx::BufferCreationDesc{
            .usage = gfx::BufferUsage::ConstantBuffer,
            .name = L"Light Constant Buffer",
        });

        m_lightsInstanceBuffer =
            graphicsDevice->createBuffer<interlop::LightInstancedRenderingBuffer>(gfx::BufferCreationDesc{
                .usage = gfx::BufferUsage::ConstantBuffer,
                .name = L"Light Instance Constant Buffer",
            });

        // Create the light model
        m_lightModel = std::make_unique<Model>(graphicsDevice, ModelCreationDesc{
                                                                   .modelPath = L"Assets/Models/Cube/glTF/Cube.gltf",
                                                                   .modelName = L"Light Model",
                                                                   .scale =
                                                                       {
                                                                           0.2f,
                                                                           0.2f,
                                                                           0.2f,
                                                                       },
                                                               });

        // Create light pipeline state.
        m_lightPipelineState = graphicsDevice->createPipelineState(gfx::GraphicsPipelineStateCreationDesc{
            .shaderModule =
                {
                    .vertexShaderPath = L"Shaders/Lights/Lights.hlsl",
                    .pixelShaderPath = L"Shaders/Lights/Lights.hlsl",
                },
            .pipelineName = L"Lights Pipeline",
        });

        // Setup directional light with intensity as 0 initially.
        m_lightsBufferData.radiusIntensity[0].y = 8.601f;
        m_lightsBufferData.lightColor[0] = {
            1.0f,
            1.0f,
            1.0f,
            1.0f,
        };

        m_lightsBufferData.numberOfLights = 1u;
    }

    void Lights::update(const math::XMMATRIX viewMatrix)
    {
        // Loop starts from 1, since the directional light will be at index 0.
        for (const uint32_t i : std::views::iota(1u, m_currentLightCount))
        {
            // Make the model matrix.

            const math::XMVECTOR translationVector = math::XMLoadFloat4(&m_lightsBufferData.lightPosition[i]);

            m_lightsInstancedBufferData.modelMatrix[i - 1] =
                math::XMMatrixScaling(m_lightsBufferData.radiusIntensity[i].x, m_lightsBufferData.radiusIntensity[i].x,
                                      m_lightsBufferData.radiusIntensity[i].x) *
                math::XMMatrixTranslationFromVector(translationVector);
        }

        for (const uint32_t i : std::views::iota(0u, m_currentLightCount))
        {
            const math::XMVECTOR positionVector = math::XMLoadFloat4(&m_lightsBufferData.lightPosition[i]);
            math::XMVECTOR viewSpaceLightPosition = math::XMVector3TransformCoord(positionVector, viewMatrix);
            if (i == 0u)
            {
                viewSpaceLightPosition = math::XMVector4Transform(positionVector, viewMatrix);
            }

            math::XMStoreFloat4(&m_lightsBufferData.viewSpaceLightPosition[i], viewSpaceLightPosition);
        }

        m_lightsBufferData.numberOfLights = m_currentLightCount;

        m_lightsInstanceBuffer.update(&m_lightsInstancedBufferData);
        m_lightsBuffer.update(&m_lightsBufferData);
    }

    void Lights::render(const gfx::GraphicsContext* const graphicsContext,
                        interlop::LightRenderResources& lightRenderResources)
    {
        graphicsContext->setGraphicsRootSignatureAndPipeline(m_lightPipelineState);

        lightRenderResources.lightBufferIndex = m_lightsBuffer.cbvIndex;
        lightRenderResources.transformBufferIndex = m_lightsInstanceBuffer.cbvIndex;

        // Subtracting one since the directional light has no visualizer and is at index 0.
        m_lightModel->render(graphicsContext, lightRenderResources, m_currentLightCount - 1u);
    }
} // namespace helios::scene