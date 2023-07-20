#include "Editor/Editor.hpp"

#include "Core/FileSystem.hpp"

#include "Scene/Scene.hpp"

#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl.h>

using namespace DirectX;

namespace helios::editor
{
    Editor::Editor(const gfx::GraphicsDevice* const graphicsDevice, SDL_Window* const window, const uint32_t width,
                   const uint32_t height)
        : m_window(window)
    {
        // Setup ImGui context.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        const auto iniFilePath = core::FileSystem::getFullPath(L"imgui.ini");
        const auto relativeIniFilePath = std::filesystem::relative(iniFilePath);
        m_iniFilePath = relativeIniFilePath.string();
        io.IniFilename = m_iniFilePath.c_str();

        // Reference : https://github.com/ocornut/imgui/blob/docking/examples/example_win32_directx12/main.cpp

        io.DisplaySize = ImVec2((float)width, (float)height);

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGuiStyle& style = ImGui::GetStyle();

        // Setup platform / renderer backends.

        ImGui_ImplSDL2_InitForD3D(window);

        gfx::DescriptorHandle srvDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getCurrentDescriptorHandle();

        ImGui_ImplDX12_Init(graphicsDevice->getDevice(), gfx::GraphicsDevice::FRAMES_IN_FLIGHT,
                            graphicsDevice->getSwapchainBackBufferFormat(),
                            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHeap(),
                            srvDescriptorHandle.cpuDescriptorHandle, srvDescriptorHandle.gpuDescriptorHandle);
        graphicsDevice->getCbvSrvUavDescriptorHeap()->offsetCurrentHandle();

        m_contentBrowserCurrentPath = core::FileSystem::getFullPath(L"Assets");
    }

    Editor::~Editor()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    // This massive class will do all rendering of UI and its settings / configs within in.
    // May seem like lot of code squashed into a single function, but this makes the engine code clean
    void Editor::render(const gfx::GraphicsDevice* const graphicsDevice,
                        gfx::GraphicsContext* const graphicsContext, scene::Scene& scene,
                        rendering::DeferredGeometryBuffer& deferredGBuffer,
                        rendering::PCFShadowMappingPass& shadowMappingPass, rendering::SSAOPass& ssaoPass,
                        rendering::BloomPass& bloomPass, interlop::PostProcessingBuffer& postProcessBuffer,
                        gfx::Texture& renderTarget)
    {
        if (m_showUI)
        {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            // Render main menu bar
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("Helios Editor"))
                {
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }

            ImGui::ShowMetricsWindow();

            // Render scene properties.
            renderSceneProperties(scene);

            // Render scene hierarchy UI.
            renderSceneHierarchy(scene);

            // Render material buffer properties.
            renderMaterialProperties(graphicsDevice, scene);

            // Render light properties.
            renderLightProperties(scene);

            // Render Deferred GBuffer data.
            renderDeferredGBuffer(graphicsDevice, deferredGBuffer);

            // Render shadow mapping pass.
            renderShadowMappingPass(graphicsDevice, shadowMappingPass);

            // Render SSAO pass.
            renderSSAOPass(graphicsDevice, ssaoPass);

            // Render Bloom pass.
            renderBloomPass(graphicsDevice, bloomPass);

            // Render post processing data.
            renderPostProcessingProperties(postProcessBuffer);

            // Render scene viewport (After all post processing).
            // All add model to model list if a path is dragged into scene viewport.
            renderSceneViewport(graphicsDevice, renderTarget, scene);

            // Render content browser panel.
            renderContentBrowser();

            // Render and update handles.
            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsContext->getCommandList());
        }
    }

    void Editor::renderSceneHierarchy(scene::Scene& scene) const
    {
        ImGui::Begin("Scene Hierarchy");

        static bool uniformScale{true};

        for (auto& [name, model] : scene.m_models)
        {
            if (ImGui::TreeNode(wStringToString(model->getName()).c_str()))
            {
                // Scale uniformally along all axises.
                ImGui::Checkbox("Uniform Scale", &uniformScale);
                ImGui::SliderFloat3("Scale", &model->getTransformComponent().scale.x, 0.1f, 15.0f);

                if (uniformScale)
                {
                    model->getTransformComponent().scale = {
                        model->getTransformComponent().scale.x,
                        model->getTransformComponent().scale.x,
                        model->getTransformComponent().scale.x,
                    };
                }

                ImGui::SliderFloat3("Translate", &model->getTransformComponent().translate.x, -100.0f, 100.0f);
                ImGui::SliderFloat3("Rotate", &model->getTransformComponent().rotation.x,
                                    DirectX::XMConvertToRadians(-180.0f), DirectX::XMConvertToRadians(180.0f));

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void Editor::renderMaterialProperties(const gfx::GraphicsDevice* const graphicsDevice,
                                          scene::Scene& scene) const
    {
        ImGui::Begin("Material Properties");

        for (auto& [name, model] : scene.m_models)
        {
            std::vector<scene::PBRMaterial>& material = model->getPBRMaterials();

            if (ImGui::TreeNode(wStringToString(model->getName()).c_str()))
            {
                for (const uint32_t i : std::views::iota(0u, material.size()))
                {
                    const std::string materialName = std::string("Material") + std::to_string(i);
                    if (ImGui::TreeNode(materialName.c_str()))
                    {
                        // note(rtarun9) : Display albedo texture, maybe this can be used to find which material we are
                        // referring to?
                        if (material[i].albedoTexture.srvIndex != INVALID_INDEX_U32)
                        {
                            const gfx::DescriptorHandle& albedoSrvHandle =
                                graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
                                    material[i].albedoTexture.srvIndex);

                            ImGui::Image((ImTextureID)(albedoSrvHandle.gpuDescriptorHandle.ptr), ImVec2(60, 60));
                        }

                        ImGui::SliderFloat("Roughness Factor", &material[i].materialBufferData.roughnessFactor, 0.0f,
                                           1.0f);
                        ImGui::SliderFloat("Metallic Factor", &material[i].materialBufferData.metallicFactor, 0.0f,
                                           1.0f);
                        ImGui::SliderFloat("Emissive Factor", &material[i].materialBufferData.emissiveFactor, 0.0f,
                                           10.0f);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void Editor::renderLightProperties(scene::Scene& scene) const
    {
        interlop::LightBuffer& lightBuffer = scene.m_lights->m_lightsBufferData;

        ImGui::Begin("Light Properties");

        if (ImGui::TreeNode("Point Lights"))
        {
            // Directional lights, if exist, starts from index 0.
            for (uint32_t pointLightIndex : std::views::iota(1u, lightBuffer.numberOfLights))
            {
                std::string name = "Point Light " + std::to_string(pointLightIndex);
                if (ImGui::TreeNode(name.c_str()))
                {
                    ImGui::ColorPicker3("Light Color", &lightBuffer.lightColor[pointLightIndex].x,
                                        ImGuiColorEditFlags_PickerHueWheel);

                    ImGui::SliderFloat3("Translate", &lightBuffer.lightPosition[pointLightIndex].x, -40.0f, 40.0f);

                    ImGui::SliderFloat("Radius", &lightBuffer.radiusIntensity[pointLightIndex].x, 0.01f, 10.0f);

                    ImGui::SliderFloat("Intensity", &lightBuffer.radiusIntensity[pointLightIndex].y, 0.1f, 30.0f);

                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Directional Lights"))
        {
            const std::string name = "Directional Light";
            constexpr uint32_t directionalLightIndex = 0u;

            DirectX::XMFLOAT4& color = lightBuffer.lightColor[0u];

            if (ImGui::TreeNode(name.c_str()))
            {
                ImGui::ColorPicker3("Light Color", &color.x,
                                    ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB |
                                        ImGuiColorEditFlags_HDR);
                ImGui::SliderFloat("Intensity", &lightBuffer.radiusIntensity[directionalLightIndex].y, 0.0f, 50.0f);
                float intensity = lightBuffer.radiusIntensity[directionalLightIndex].y;

                lightBuffer.lightColor[directionalLightIndex] = {color.x, color.y, color.z, color.w};

                static float sunAngle{scene::Lights::DIRECTIONAL_LIGHT_ANGLE};
                ImGui::SliderFloat("Sun Angle", &sunAngle, -180.0f, 180.0f);
                lightBuffer.lightPosition[directionalLightIndex] = math::XMFLOAT4(
                    0.0f, sin(math::XMConvertToRadians(sunAngle)), cos(math::XMConvertToRadians(sunAngle)), 0.0f);

                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        ImGui::End();
    }

    void Editor::renderSceneProperties(scene::Scene& scene) const
    {
        ImGui::Begin("Scene Properties");

        if (ImGui::TreeNode("Primary Camera"))
        {
            // Scale uniformally along all axises.
            ImGui::SliderFloat("Movement Speed", &scene.m_camera.m_movementSpeed, 0.1f, 3.0f);
            ImGui::SliderFloat("Rotation Speed", &scene.m_camera.m_rotationSpeed, 0.1f, 1.0f);

            ImGui::SliderFloat("Friction Factor", &scene.m_camera.m_frictionFactor, 0.0f, 1.0f);
            //	ImGui::SliderFloat("Sun Angle Speed", &scene->mSunChangeSpeed, -100.0f, 50.0f);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Projection Params"))
        {
            ImGui::SliderFloat("FOV", &scene.m_fov, 0.5f, 90.0f);
            ImGui::SliderFloat("Near Plane", &scene.m_nearPlane, 0.1f, 100.0f);
            ImGui::SliderFloat("Far Plane", &scene.m_farPlane, 10.0f, 10000.0f);

            ImGui::TreePop();
        }

        ImGui::End();
    }

    void Editor::renderDeferredGBuffer(const gfx::GraphicsDevice* const graphicsDevice,
                                       const rendering::DeferredGeometryBuffer& deferredGBuffer) const
    {
        // Only the albedo portion of the gbuffer is being rendered.
        // This is because the other render targets have either negative values, or the last component is rarely 1,
        // leading to the ImGui::Image to simply be transparent. Uncommenting these lines will make the entire GBuffer
        // viewable from the editor.
        const gfx::DescriptorHandle& albedoEmissiveDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
                deferredGBuffer.albedoEmissiveRT.srvIndex);
        // const gfx::DescriptorHandle& normalEmissiveDescriptorHandle =
        //     graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
        //         deferredGBuffer.normalEmissiveRT.srvIndex);
        // const gfx::DescriptorHandle& aoMetalRoughnessDescriptorHandle =
        //     graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
        //         deferredGBuffer.aoMetalRoughnessEmissiveRT.srvIndex);

        ImGui::Begin("Albedo RT");
        ImGui::Image((ImTextureID)(albedoEmissiveDescriptorHandle.gpuDescriptorHandle.ptr),
                     ImGui::GetWindowViewport()->WorkSize);
        ImGui::End();

        // ImGui::Begin("Normal RT");
        // ImGui::Image((ImTextureID)(normalEmissiveDescriptorHandle.cpuDescriptorHandle.ptr),
        //              ImGui::GetWindowViewport()->WorkSize);
        // ImGui::End();
        //
        // ImGui::Begin("AO Metal Roughness RT");
        // ImGui::Image((ImTextureID)(aoMetalRoughnessDescriptorHandle.cpuDescriptorHandle.ptr),
        //              ImGui::GetWindowViewport()->WorkSize);
        // ImGui::End();
        //
        // ImGui::Begin("Position RT");
        // ImGui::Image((ImTextureID)(positionEmissiveDescriptorHandle.cpuDescriptorHandle.ptr),
        //              ImGui::GetWindowViewport()->WorkSize);
        // ImGui::End();
    }

    void Editor::renderShadowMappingPass(const gfx::GraphicsDevice* const graphicsDevice,
                                         rendering::PCFShadowMappingPass& shadowMappingPass) const
    {
        ImGui::Begin("Shadow Pass");
        const auto srvDescriptorHandle = graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
            shadowMappingPass.m_shadowDepthBuffer.srvIndex);

        ImGui::Begin("Shadow Depth Map");
        ImGui::Image((ImTextureID)(srvDescriptorHandle.gpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);
        ImGui::End();

        ImGui::SliderFloat("Backoff distance", &shadowMappingPass.m_shadowBufferData.backOffDistance, 0.0f, 300.0f);
        ImGui::SliderFloat("Extents", &shadowMappingPass.m_shadowBufferData.extents, 0.0f, 300.0f);
        ImGui::SliderFloat("Near Plane", &shadowMappingPass.m_shadowBufferData.nearPlane, 0.0f, 10.0f);
        ImGui::SliderFloat("Far Plane", &shadowMappingPass.m_shadowBufferData.farPlane, 50.0f, 500.0f);

        ImGui::End();
    }

    void Editor::renderSSAOPass(const gfx::GraphicsDevice* const graphicsDevice, rendering::SSAOPass& ssaoPass) const
    {
        ImGui::Begin("SSAO Pass");

        const auto blurSSAOTextureSrvDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
                ssaoPass.m_blurSSAOTexture.srvIndex);

        ImGui::Begin("SSAO Blur Texture");
        ImGui::Image((ImTextureID)(blurSSAOTextureSrvDescriptorHandle.gpuDescriptorHandle.ptr),
                     ImGui::GetWindowViewport()->WorkSize);
        ImGui::End();

        ImGui::SliderFloat("Bias", &ssaoPass.m_ssaoBufferData.bias, 0.0f, 1.0f);
        ImGui::SliderFloat("Radius", &ssaoPass.m_ssaoBufferData.radius, 0.0f, 10.0f);
        ImGui::SliderFloat("Power", &ssaoPass.m_ssaoBufferData.power, 0.0f, 10.0f);
        ImGui::SliderFloat("Occlusion Multiplier", &ssaoPass.m_ssaoBufferData.occlusionMultiplier, 1.0f, 5.0f);

        ImGui::End();
    }

    void Editor::renderBloomPass(const gfx::GraphicsDevice* const graphicsDevice, rendering::BloomPass& bloomPass) const
    {
        ImGui::Begin("Bloom Pass");

        ImGui::SliderFloat("Threshold", &bloomPass.m_bloomBufferData.threshHold, 0.0f, 10.0f);
        ImGui::SliderFloat("Radius", &bloomPass.m_bloomBufferData.radius, 0.0f, 10.0f);

         const auto bloomExtractTextureSrvDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
                bloomPass.m_extractionTexture.srvIndex);

         ImGui::Begin("Bloom Extract Texture");
         ImGui::Image((ImTextureID)(bloomExtractTextureSrvDescriptorHandle.gpuDescriptorHandle.ptr),
                      ImGui::GetMainViewport()->WorkSize);
         ImGui::End();

        static int bloomDownSampleMipIndex{0};
        ImGui::SliderInt("DownSample Mip Level", &bloomDownSampleMipIndex, 0, interlop::BLOOM_PASSES-1);

        const auto bloomDownPassTextureSrvDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
                bloomPass.m_bloomDownSampleTexture.srvIndex + bloomDownSampleMipIndex);

        ImGui::Begin("Bloom Downsample Texture");
        ImGui::Image((ImTextureID)(bloomDownPassTextureSrvDescriptorHandle.gpuDescriptorHandle.ptr),
                     ImGui::GetWindowViewport()->WorkSize);
        ImGui::End();

         static int bloomUpSampleMipIndex{0};
        ImGui::SliderInt("UpSample Mip Level", &bloomUpSampleMipIndex, 0, interlop::BLOOM_PASSES - 1);

        const auto bloomUpSampleTextureSrvDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
                bloomPass.m_bloomUpSampleTexture.srvIndex + bloomUpSampleMipIndex);

        ImGui::Begin("Bloom UpSample Texture");
        ImGui::Image((ImTextureID)(bloomUpSampleTextureSrvDescriptorHandle.gpuDescriptorHandle.ptr),
                     ImGui::GetWindowViewport()->WorkSize);
        ImGui::End();

        ImGui::End();
    }

    void Editor::renderPostProcessingProperties(interlop::PostProcessingBuffer& postProcessBufferData) const
    {
        ImGui::Begin("Post Processing Properties");
        static int showSSAOTexture{};
        ImGui::SliderInt("Show SSAO Texture", &showSSAOTexture, 0, 1);
        postProcessBufferData.debugShowSSAOTexture = static_cast<uint32_t>(showSSAOTexture);
        static int enableBloom{1};
        ImGui::SliderInt("Enable Bloom", &enableBloom, 0, 1);
        postProcessBufferData.enableBloom = static_cast<uint32_t>(enableBloom);
        ImGui::SliderFloat("Bloom Strength", &postProcessBufferData.bloomStrength, 0.0f, 1.0f);
        ImGui::End();
    }

    void Editor::renderSceneViewport(const gfx::GraphicsDevice* const graphicsDevice, gfx::Texture& renderTarget,
                                     scene::Scene& scene) const
    {
        const gfx::DescriptorHandle& rtvSrvHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(renderTarget.srvIndex);

        ImGui::Begin("View Port");
        ImGui::Image((ImTextureID)(rtvSrvHandle.gpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);

        // Handling the drag-drop facility for GLTF models.
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payLoad =
                    ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET_ITEM", ImGuiCond_Once))
            {
                // Check if item path dragged in actually belongs to model. If yes, load the model and add to models
                // vector. If yes, let the model name be the name between final \ and .gltf. note(rtarun9) : the / and
                // \\ are platform specific, preferring windows format for now.
                const wchar_t* modelPath = reinterpret_cast<const wchar_t*>(payLoad->Data);
                if (std::wstring modelPathWStr = modelPath; modelPathWStr.find(L".gltf") != std::wstring::npos ||
                                                            modelPathWStr.find(L".glb") != std::wstring::npos)
                {
                    // Needed if two models have same name they end up having same transform.
                    static int modelNumber{};

                    const size_t lastSlash = modelPathWStr.find_last_of(L"\\");
                    const size_t lastDot = modelPathWStr.find_last_of(L".");
                    const std::wstring modelName =
                        modelPathWStr.substr(lastSlash + 1, lastDot - lastSlash) + std::to_wstring(modelNumber++);

                    // note(rtarun9) : the +1 and -1 are present to get the exact name (example : \\test.gltf should
                    // return test.
                    const scene::ModelCreationDesc modelCreationDesc = {
                        .modelPath = modelPathWStr,
                        .modelName = modelName,
                    };

                    scene.addModel(graphicsDevice, modelCreationDesc);
                    scene.completeResourceLoading();
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::End();
    }

    void Editor::renderContentBrowser()
    {
        const auto assetsPath = core::FileSystem::getFullPath(L"Assets/Models");
        ImGui::Begin("Content Browser");

        // We dont want to allow renderer to see paths other than those of the Assets directory.
        // If the current path is not assets (i.e within one of the assets/ files), show a backbutton, else disable it.
        if (m_contentBrowserCurrentPath != assetsPath)
        {
            if (ImGui::Button("Back"))
            {
                m_contentBrowserCurrentPath = m_contentBrowserCurrentPath.parent_path();
            }
        }

        // If a directory entry is a path (folder), let it be a button which on pressing will set the current content
        // browser path to go within the folder. Else, just display the file name as text.
        for (auto& directoryEntry : std::filesystem::directory_iterator(m_contentBrowserCurrentPath))
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                // If a asset (in our case, model) is being dragged, payload is set to its path.
                // Null size is + 1 because nulltermination char must also be copied.

                auto itemPath = directoryEntry.path().wstring();
                auto itemPathStr = itemPath.c_str();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET_ITEM", itemPathStr,
                                          (wcslen(itemPathStr) + 1) * sizeof(wchar_t));
                ImGui::EndDragDropSource();
            }

            if (directoryEntry.is_directory())
            {
                if (ImGui::Button(directoryEntry.path().string().c_str()))
                {
                    // Weird C++ operator for concatanating two paths.
                    m_contentBrowserCurrentPath /= directoryEntry.path().filename();
                }
            }
            else
            {
                ImGui::TextColored({0.9f, 0.9f, 0.8f, 1.0f}, "%s\n", directoryEntry.path().string().c_str());
            }
        }

        ImGui::End();
    }

    void Editor::showUI(bool value)
    {
        m_showUI = value;
    }

    void Editor::resize(const uint32_t width, const uint32_t height) const
    {
        ImGui::GetMainViewport()->WorkSize = ImVec2((float)width, (float)height);
        ImGui::GetMainViewport()->Size = ImVec2((float)width, (float)height);
    }
} // namespace helios::editor