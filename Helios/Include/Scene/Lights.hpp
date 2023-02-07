#pragma once

#include "Graphics/Resources.hpp"
#include "Graphics/PipelineState.hpp"

#include "Model.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
}

namespace helios::scene
{    
    enum class LightTypes : uint8_t
    {
        PointLightData,
        DirectionalLightData
    };

    struct LightCreationDesc
    {
        LightTypes lightType{};
    };

    // Lights is the Light Manager abstraction for all lights in the engine.
    // This common light abstraction is used for all types of Light (Point, directional, area, punctual etc in the Engine.
    // This is also why there is a common buffer for all light types. Instanced rendering is used to visualize the lights.
    // The number of lights is currently fixed, and set in ShaderInterlop/ConstantBuffers.hlsli.
    class Lights
    {
      public:
        explicit Lights(const gfx::GraphicsDevice* const graphicsDevice);
        
        // Update light buffer and update the model matrices (to be used in the Instanced rendering buffer).
        void update();

        // Render all visualizable lights in a instanced rendering fashion.
        void render(const gfx::GraphicsContext* graphicsContext, interlop::LightRenderResources& lightRenderResources);

      public:
        static constexpr float DIRECTIONAL_LIGHT_ANGLE{-99.0f};

      public:
        // Store light positions, color, intensities, etc.
        interlop::LightBuffer m_lightsBufferData{};
        gfx::Buffer m_lightsBuffer{};

        // Store the model matrices required for instanced rendering.
        interlop::LightInstancedRenderingBuffer m_lightsInstancedBufferData{};
        gfx::Buffer m_lightsInstanceBuffer{};

        // All lights (point) will use a same mesh.
        // However, note that each light has its own model matrices, as instanced rendering will be used for light
        // visualization.
        std::unique_ptr<Model> m_lightModel;

        uint32_t m_currentLightCount{};

        gfx::PipelineState m_lightPipelineState{};
    };
} // namespace helios::scene