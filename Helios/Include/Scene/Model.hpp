#pragma once

#include "Materials.hpp"
#include "Mesh.hpp"

#include "../Graphics/Resources.hpp"

#include <tiny_gltf.h>

#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

namespace helios::gfx
{
    class GraphicsDevice;
    class GraphicsContext;
} // namespace helios::gfx

namespace helios::scene
{
    struct TransformComponent
    {
        math::XMFLOAT3 rotation{0.0f, 0.0f, 0.0f};
        math::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
        math::XMFLOAT3 translate{0.0f, 0.0f, 0.0f};

        gfx::Buffer transformBuffer{};

        void update();
    };

    struct ModelCreationDesc
    {
        std::wstring_view modelPath{};
        std::wstring_view modelName{};

        math::XMFLOAT3 rotation{0.0f, 0.0f, 0.0f};
        math::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
        math::XMFLOAT3 translate{0.0f, 0.0f, 0.0f};
    };

    // Model class uses tinygltf for loading GLTF models.
    // Currently, only GLTF model loading is supported. This is mostly because of the much faster load times of this
    // mesh type compared to .obj, .fbx, etc.
    // note(rtarun9) : For now, the Model will have ownership of meshes and materials, in future move these to the
    // ResourceManager and just obtain pointers to them, hence sharing them between all models.
    class Model
    {
      public:
        Model() = default;
        Model(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc);

        TransformComponent& getTransformComponent()
        {
            return m_transformComponent;
        };

        std::vector<PBRMaterial>& getPBRMaterials()
        {
            return m_materials;
        }

        const std::wstring_view getName() const
        {
            return m_modelName;
        }

        void updateMaterialBuffer();

        void render(const gfx::GraphicsContext* const graphicsContext,
                    interlop::ModelViewerRenderResources& renderResources) const;

        void render(const gfx::GraphicsContext* const graphicsContext,
                    interlop::BlinnPhongRenderResources& renderResources) const;

        void render(const gfx::GraphicsContext* const graphicsContext,
                    interlop::DeferredGPassRenderResources& renderResources) const;

        void render(const gfx::GraphicsContext* const graphicsContext,
                    interlop::CubeMapRenderResources& renderResources) const;

        void render(const gfx::GraphicsContext* const graphicsContext,
                    interlop::ShadowPassRenderResources& renderResources) const;

        void render(const gfx::GraphicsContext* const graphicsContext, interlop::LightRenderResources& renderResources,
                    const uint32_t lightInstancesCount) const;

      private:
        void loadNode(const gfx::GraphicsDevice* const graphicsDevice, const ModelCreationDesc& modelCreationDesc,
                      const uint32_t nodeIndex, const tinygltf::Model& model);
        void loadSamplers(const gfx::GraphicsDevice* const graphicsDevice, const tinygltf::Model& model);
        void loadMaterials(const gfx::GraphicsDevice* const graphicsDevice, const tinygltf::Model& model);

        TransformComponent m_transformComponent{};

      public:
        std::wstring m_modelName{};

      private:
        std::vector<Mesh> m_meshes{};
        std::vector<PBRMaterial> m_materials{};
        std::vector<gfx::Sampler> m_samplers{};

        std::wstring m_modelPath{};
        std::wstring m_modelDirectory{};
    };
} // namespace helios::scene