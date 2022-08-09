#include "Pch.hpp"
#include "Editor.hpp"

#include "Core/Application.hpp"

#include "Scene/Scene.hpp"

#include "Graphics/RenderPass/DeferredGeometryPass.hpp"

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx12.h"
#include "ImGUI/imgui_impl_win32.h"

namespace helios::editor
{
	Editor::Editor(const gfx::Device* device)
	{
		ImGui_ImplWin32_EnableDpiAwareness();

		// Setup ImGui context.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Reference : https://github.com/ocornut/imgui/blob/docking/examples/example_win32_directx12/main.cpp

		io.DisplaySize = ImVec2((float)core::Application::GetClientDimensions().x, (float)core::Application::GetClientDimensions().y);
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		
		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Set up cinder dark theme (from : https://github.com/GraphicsProgramming/dear-imgui-styles)
		SetCustomDarkTheme();

		// Setup platform / renderer backends.

		ImGui_ImplWin32_Init(core::Application::GetWindowHandle());
		gfx::DescriptorHandle srvDescriptorHandle = device->GetSrvCbvUavDescriptor()->GetCurrentDescriptorHandle();

		ImGui_ImplDX12_Init(device->GetDevice(), gfx::Device::NUMBER_OF_FRAMES, gfx::Device::SWAPCHAIN_FORMAT, device->GetSrvCbvUavDescriptor()->GetDescriptorHeap(), srvDescriptorHandle.cpuDescriptorHandle, srvDescriptorHandle.gpuDescriptorHandle);
		device->GetSrvCbvUavDescriptor()->OffsetCurrentHandle();
	}

	Editor::~Editor()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	// This massive class will do all rendering of UI and its settings / configs within in.
	// May seem like lot of code squashed into a single function, but this makes the engine code clean
	void Editor::Render(gfx::Device* device, scene::Scene* scene, gfx::DeferredPassRTs* deferredPassRTs, std::span<float, 4> clearColor, PostProcessBuffer& postProcessBufferData, gfx::DescriptorHandle rtDescriptorHandle, gfx::GraphicsContext* graphicsContext)
	{
		if (mShowUI)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
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
			RendererProperties(clearColor, postProcessBufferData);

			// Render camera UI.
			RenderSceneProperties(scene);

			// Render scene hierarchy UI.
			RenderSceneHierarchy(scene->mModels);

			// Render Light property menu.
			RenderLightProperties(scene->mLights);

			// Render deferred pass rt's.
			RenderDeferredGPass(device, deferredPassRTs);

			// Render scene viewport (After all post processing).
			// All add model to model list if a path is dragged into scene viewport.
			RenderSceneViewport(device, rtDescriptorHandle, scene);

			// Render content browser panel.
			RenderContentBrowser();

			// Render and update handles.
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsContext->GetCommandList());

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault(core::Application::GetWindowHandle(), graphicsContext->GetCommandList());
			}
		}
	}

	void Editor::SetCustomDarkTheme() const
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.73f);
		style.WindowMinSize = ImVec2(160, 20);
		style.FramePadding = ImVec2(4, 2);
		style.ItemSpacing = ImVec2(6, 2);
		style.ItemInnerSpacing = ImVec2(6, 4);
		style.Alpha = 0.95f;
		style.WindowRounding = 4.0f;
		style.FrameRounding = 2.0f;
		style.IndentSpacing = 6.0f;
		style.ItemInnerSpacing = ImVec2(2, 4);
		style.ColumnsMinSpacing = 50.0f;
		style.GrabMinSize = 14.0f;
		style.GrabRounding = 16.0f;
		style.ScrollbarSize = 12.0f;
		style.ScrollbarRounding = 16.0f;
	}

	void Editor::RenderSceneHierarchy(std::span<std::unique_ptr<helios::scene::Model>> models) const
	{
		ImGui::Begin("Scene Hierarchy");
		
		for (auto& model : models)
		{
			if (ImGui::TreeNode(WstringToString(model->GetName()).c_str()))
			{
				// Scale uniformally along all axises.
				ImGui::SliderFloat("Scale", &model->GetTransform()->data.scale.x, 0.1f, 15.0f);

				model->GetTransform()->data.scale = math::XMFLOAT3(model->GetTransform()->data.scale.x, model->GetTransform()->data.scale.x, model->GetTransform()->data.scale.x);

				ImGui::SliderFloat3("Translate", &model->GetTransform()->data.translate.x, -10.0f, 10.0f);
				ImGui::SliderFloat3("Rotate", &model->GetTransform()->data.rotation.x, DirectX::XMConvertToRadians(-180.0f), DirectX::XMConvertToRadians(180.0f));

				ImGui::TreePop();
			}
		}		

		ImGui::End();
	}

	void Editor::RenderSceneProperties(scene::Scene* scene) const
	{
		ImGui::Begin("Scene Properties");

		if (ImGui::TreeNode("Primary Camera"))
		{
			// Scale uniformally along all axises.
			ImGui::SliderFloat("Movement Speed", &scene->mCamera->mMovementSpeed, 0.1f, 5000.0f);
			ImGui::SliderFloat("Rotation Speed", &scene->mCamera->mRotationSpeed, 0.1f, 250.0f);

			ImGui::SliderFloat("Friction Factor", &scene->mCamera->mFrictionFactor, 0.0f, 1.0f);
			
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Projection"))
		{
			ImGui::SliderFloat("FOV", &scene->mFov, 0.5f, 90.0f);
			ImGui::SliderFloat("Near Plane", &scene->mNearPlane, 0.1f, 100.0f);
			ImGui::SliderFloat("Far Plane", &scene->mFarPlane, 10.0f, 10000.0f);

			ImGui::TreePop();
		}

		ImGui::End();
	}

	void Editor::RendererProperties(std::span<float, 4> clearColor, PostProcessBuffer& postProcessBufferData) const
	{
		ImGui::Begin("Scene Properties");
		ImGui::ColorPicker3("Clear Color", clearColor.data(), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
		ImGui::SliderFloat("Exposure", &postProcessBufferData.exposure, 0.0f, 5.0f);
		ImGui::End();
	}

	void Editor::RenderLightProperties(std::vector<std::unique_ptr<helios::scene::Light>>& lights) const
	{
		ImGui::Begin("Light Properties");

		if (ImGui::TreeNode("Point Lights"))
		{
			// Taking advantage of the fact that point lights come before directional lights (see Common/ConstantBuffers.hlsli for more info).
			for (uint32_t pointLightIndex : std::views::iota(0u, TOTAL_POINT_LIGHTS))
			{
				std::string name = "Point Light " + std::to_string(pointLightIndex);
				if (ImGui::TreeNode(name.c_str()))
				{
					ImGui::ColorPicker3("Light Color", &scene::Light::GetLightBufferData()->lightColor[pointLightIndex].x, ImGuiColorEditFlags_PickerHueWheel);

					ImGui::SliderFloat3("Translate", &scene::Light::GetLightBufferData()->lightPosition[pointLightIndex].x, -40.0f, 40.0f);
					
					ImGui::SliderFloat("Radius", &scene::Light::GetLightBufferData()->radius[pointLightIndex].x, 0.01f, 10.0f);

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}


		if (ImGui::TreeNode("Directional Lights"))
		{
			for (uint32_t directionalLightIndex : std::views::iota(DIRECTIONAL_LIGHT_OFFSET, DIRECTIONAL_LIGHT_OFFSET + TOTAL_DIRECTIONAL_LIGHTS))
			{
				std::string name = "Directional Light " + std::to_string(directionalLightIndex);
				if (ImGui::TreeNode(name.c_str()))
				{
					ImGui::ColorPicker3("Light Color", &scene::Light::GetLightBufferData()->lightColor[directionalLightIndex].x, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);

					static float sunAngle{ scene::Light::DIRECTIONAL_LIGHT_ANGLE };
					ImGui::SliderFloat("Sun Angle", &sunAngle, -180.0f, 180.0f);
					scene::Light::GetLightBufferData()->lightPosition[directionalLightIndex] = math::XMFLOAT4(0.0f, sin(math::XMConvertToRadians(sunAngle)), cos(math::XMConvertToRadians(sunAngle)), 0.0f);

					ImGui::TreePop();
				}
			}


			ImGui::TreePop();
		}

		ImGui::End();
	}

	void Editor::RenderDeferredGPass(gfx::Device* device, const gfx::DeferredPassRTs* deferredRTs) const
	{
		gfx::DescriptorHandle positionDescriptorHandle = device->GetTextureSrvDescriptorHandle(deferredRTs->positionRT->renderTexture.get());
		gfx::DescriptorHandle normalDescriptorHandle = device->GetTextureSrvDescriptorHandle(deferredRTs->normalRT->renderTexture.get());
		gfx::DescriptorHandle albedoDescriptorHandle = device->GetTextureSrvDescriptorHandle(deferredRTs->albedoRT->renderTexture.get());

		ImGui::Begin("Position RT");
		ImGui::Image((ImTextureID)(positionDescriptorHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();

		ImGui::Begin("Normal RT");
		ImGui::Image((ImTextureID)(normalDescriptorHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();

		ImGui::Begin("Albedo RT");
		ImGui::Image((ImTextureID)(albedoDescriptorHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();
	}

	void Editor::RenderSceneViewport(const gfx::Device* device, gfx::DescriptorHandle rtDescriptorHandle, scene::Scene* scene) const
	{
		ImGui::Begin("View Port");
		ImGui::Image((ImTextureID)(rtDescriptorHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payLoad = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET_ITEM", ImGuiCond_Once))
			{
				// Check if item path dragged in actually belongs to model. If yes, load the model and add to models vector.
				// If yes, let the model name be the name between final \ and .gltf.
				// note(rtarun9) : the / and \\ are platform specific, preferring windows format for now.
				const wchar_t* modelPath = reinterpret_cast<const wchar_t*>(payLoad->Data);
				if (std::wstring modelPathWStr = modelPath; modelPathWStr.find(L".gltf") != std::wstring::npos)
				{
					// Needed if two models have same name they end up having same transform.
					static int modelNumber{};
					size_t lastSlash = modelPathWStr.find_last_of(L"\\");
					size_t lastDot = modelPathWStr.find_last_of(L".");

					// note(rtarun9) : the +1 and -1 are present to get the exact name (example : \\test.gltf should return test.
					scene::ModelCreationDesc modelCreationDesc
					{
						.modelPath = modelPathWStr,
						.modelName = modelPathWStr.substr(lastSlash + 1, lastDot - lastSlash - 1) + std::to_wstring(modelNumber++),
					};

					scene->AddModel(device, modelCreationDesc);
				}
			}


			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void Editor::RenderContentBrowser()
	{
		ImGui::Begin("Content Browser");

		// We dont want to allow renderer to see paths other than those of the Assets directory.
		// If the current path is not assets (i.e within one of the assets/ files), show a backbutton, else disable it.
		if (mContentBrowserCurrentPath != ASSETS_PATH)
		{
			if (ImGui::Button("Back"))
			{
				mContentBrowserCurrentPath = mContentBrowserCurrentPath.parent_path();
			}
		}

		// If a directory entry is a path (folder), let it be a button which on pressing will set the current content browser path to go within the folder.
		// Else, just display the file name as text.
		for (auto& directoryEntry : std::filesystem::directory_iterator(mContentBrowserCurrentPath))
		{
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				// If a asset (in our case, model) is being dragged, payload is set to its path.
				// Null size is + 1 because nulltermination char must also be copied.
				std::filesystem::path absolutePath = std::filesystem::absolute(directoryEntry.path());
				const wchar_t* itemPath = absolutePath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
				ImGui::EndDragDropSource();
			}

			if (directoryEntry.is_directory())
			{
				if (ImGui::Button(directoryEntry.path().string().c_str()))
				{
					// Weird C++ operator for concatanating two paths.
					mContentBrowserCurrentPath /= directoryEntry.path().filename();
				}
			}
			else
			{
				ImGui::TextColored({ 0.9f, 0.9f, 0.8f, 1.0f }, "%s\n", directoryEntry.path().string().c_str());
			}
		}

		ImGui::End();
	}

	void Editor::ShowUI(bool value)
	{
		mShowUI = value;
	}

	void Editor::OnResize(Uint2 dimensions) const
	{
		ImGui::GetMainViewport()->WorkSize = ImVec2((float)dimensions.x, (float)dimensions.y);
		ImGui::GetMainViewport()->Size = ImVec2((float)dimensions.x, (float)dimensions.y);
	}
}
