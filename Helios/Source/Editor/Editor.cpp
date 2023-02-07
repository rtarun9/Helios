#include "Editor/Editor.hpp"

#include "Core/ResourceManager.hpp"

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
        const auto iniFilePath = core::ResourceManager::getFullPath(L"imgui.ini");
        const auto relativeIniFilePath = std::filesystem::relative(iniFilePath);
        m_iniFilePath = relativeIniFilePath.string();
        io.IniFilename = m_iniFilePath.c_str();

        // Reference : https://github.com/ocornut/imgui/blob/docking/examples/example_win32_directx12/main.cpp

        io.DisplaySize = ImVec2((float)width, (float)height);

        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular
        // ones.
        ImGuiStyle& style = ImGui::GetStyle();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup platform / renderer backends.

        ImGui_ImplSDL2_InitForD3D(window);

        gfx::DescriptorHandle srvDescriptorHandle =
            graphicsDevice->getCbvSrvUavDescriptorHeap()->getCurrentDescriptorHandle();

        ImGui_ImplDX12_Init(graphicsDevice->getDevice(), gfx::GraphicsDevice::FRAMES_IN_FLIGHT,
                            graphicsDevice->getSwapchainBackBufferFormat(),
                            graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHeap(),
                            srvDescriptorHandle.cpuDescriptorHandle, srvDescriptorHandle.gpuDescriptorHandle);
        graphicsDevice->getCbvSrvUavDescriptorHeap()->offsetCurrentHandle();

        m_contentBrowserCurrentPath = core::ResourceManager::getFullPath(L"Assets");
    }

    Editor::~Editor()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    // This massive class will do all rendering of UI and its settings / configs within in.
    // May seem like lot of code squashed into a single function, but this makes the engine code clean
    void Editor::render(const gfx::GraphicsDevice* const graphicsDevice, scene::Scene* const scene,
                        const std::span<float, 4> clearColor, gfx::Texture* const renderTarget,
                        gfx::GraphicsContext* const graphicsContext)
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

            // Set clear color & other scene properties.
            rendererProperties(clearColor);

            // Render scene properties.
            renderSceneProperties(scene);

            // Render scene hierarchy UI.
            renderSceneHierarchy(scene);

            // Render scene viewport (After all post processing).
            // All add model to model list if a path is dragged into scene viewport.
            // note(rtarun9) : renderSceneViewport is yet to be completed with implementation.
            //renderSceneViewport(graphicsDevice, renderTarget, scene);

            // Render content browser panel.
            renderContentBrowser();

            // Render and update handles.
            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsContext->getCommandList());

            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault(m_window, graphicsContext->getCommandList());
            }
        }
    }

    void Editor::renderSceneHierarchy(scene::Scene* const scene) const
    {
        ImGui::Begin("Scene Hierarchy");

        static bool uniformScale{true};

        for (auto& [name, model] : scene->m_models)
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

                ImGui::SliderFloat3("Translate", &model->getTransformComponent().translate.x, -10.0f, 10.0f);
                ImGui::SliderFloat3("Rotate", &model->getTransformComponent().rotation.x,
                                    DirectX::XMConvertToRadians(-180.0f), DirectX::XMConvertToRadians(180.0f));

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void Editor::renderSceneProperties(scene::Scene* scene) const
    {
        ImGui::Begin("Scene Properties");

        if (ImGui::TreeNode("Primary Camera"))
        {
            // Scale uniformally along all axises.
            ImGui::SliderFloat("Movement Speed", &scene->m_camera.m_movementSpeed, 0.1f, 3.0f);
            ImGui::SliderFloat("Rotation Speed", &scene->m_camera.m_rotationSpeed, 0.1f, 1.0f);

            ImGui::SliderFloat("Friction Factor", &scene->m_camera.m_frictionFactor, 0.0f, 1.0f);
            //	ImGui::SliderFloat("Sun Angle Speed", &scene->mSunChangeSpeed, -100.0f, 50.0f);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Projection"))
        {
            ImGui::SliderFloat("FOV", &scene->m_fov, 0.5f, 90.0f);
            ImGui::SliderFloat("Near Plane", &scene->m_nearPlane, 0.1f, 100.0f);
            ImGui::SliderFloat("Far Plane", &scene->m_farPlane, 10.0f, 10000.0f);

            ImGui::TreePop();
        }

        ImGui::End();
    }

    void Editor::rendererProperties(const std::span<float, 4> clearColor) const
    {
        ImGui::Begin("Scene Properties");
        ImGui::ColorPicker3("Clear Color", clearColor.data(),
                            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
        ImGui::End();
    }

    void Editor::renderSceneViewport(const gfx::GraphicsDevice* const graphicsDevice, gfx::Texture* const renderTarget,
                                     scene::Scene* const scene) const
    {
        const gfx::DescriptorHandle& rtvSrvHandle = graphicsDevice->getCbvSrvUavDescriptorHeap()->getDescriptorHandleFromIndex(
            renderTarget->srvIndex);

        ImGui::Begin("View Port");
        ImGui::Image((ImTextureID)(rtvSrvHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);

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

                    size_t lastSlash = modelPathWStr.find_last_of(L"\\");
                    size_t lastDot = modelPathWStr.find_last_of(L".");

                    // note(rtarun9) : the +1 and -1 are present to get the exact name (example : \\test.gltf should
                    // return test.
                    scene::ModelCreationDesc modelCreationDesc{
                        .modelPath = modelPathWStr,
                        .modelName = modelPathWStr.substr(lastSlash + 1, lastDot - lastSlash - 1) +
                                     std::to_wstring(modelNumber++),
                    };

                    scene->addModel(graphicsDevice, modelCreationDesc);
                    scene->completeResourceLoading();
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::End();
    }

    void Editor::renderContentBrowser()
    {
        const auto assetsPath = core::ResourceManager::getFullPath(L"Assets/Models");
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