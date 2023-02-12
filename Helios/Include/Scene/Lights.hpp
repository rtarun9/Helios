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
    // The max number of lights is currently fixed, and set in ShaderInterlop/ConstantBuffers.hlsli.
    // The engine will always have a directional light, but whose intensity is set to 0 at start of engine.
    class Lights
    {
      public:
        explicit Lights(const gfx::GraphicsDevice* const graphicsDevice);
        
        // Update light buffer and update the model matrices (to be used in the Instanced rendering buffer).
        void update(const math::XMMATRIX viewMatrix);

        // Render all visualizable lights in a instanced rendering fashion.
        void render(const gfx::GraphicsContext* graphicsContext, interlop::LightRenderResources& lightRenderResources);

      public:
        static constexpr float DIRECTIONAL_LIGHT_ANGLE{123.0f};

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

        // Directional light always exist, so current light count starts from 1.
        uint32_t m_currentLightCount{1u};

        // Pipeline state to be used for rendering lights.
        gfx::PipelineState m_lightPipelineState{};
    };
} // namespace helios::scene