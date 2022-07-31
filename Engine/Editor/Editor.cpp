#include "Pch.hpp"
#include "Editor.hpp"

#include "Core/Application.hpp"

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

		// note(rtarun9 : Uncommenting this line causes some issues (because of the HWND being null, and ImGui internally creates a new window for this).
		// Currently, multiple viewports is not being used so commenting this out.
		// Error we recieve upon shutdown (closing window) : 
		// Line: 850
		//Expression: hwnd != 0

		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

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

		ImGui_ImplDX12_Init(device->GetDevice(), gfx::Device::NUMBER_OF_FRAMES, DXGI_FORMAT_R8G8B8A8_UNORM, device->GetSrvCbvUavDescriptor()->GetDescriptorHeap(), srvDescriptorHandle.cpuDescriptorHandle, srvDescriptorHandle.gpuDescriptorHandle);
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
	// and when ECS is setup, it should take only one paramter and make the solution a bit cleaner.
	void Editor::Render(std::span<std::unique_ptr<helios::scene::Model>> models, scene::Camera* camera, std::span<float, 4> clearColor, gfx::DescriptorHandle rtDescriptorHandle, gfx::GraphicsContext* graphicsContext) const
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

			// Set clear color & other scene properties.
			RenderSceneProperties(clearColor);

			// Render scene hierarchy UI.
			RenderModelProperties(models);

			// Render camera UI.
			RenderCameraProperties(camera);

			// Render scene viewport (After all post processing).
			RenderSceneViewport(rtDescriptorHandle);

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

	void Editor::RenderModelProperties(std::span<std::unique_ptr<helios::scene::Model>> models) const
	{
		ImGui::Begin("Scene Hierarchy");
		
		for (auto& model : models)
		{
			if (ImGui::TreeNode(WstringToString(model->GetName()).c_str()))
			{
				// Scale uniformally along all axises.
				ImGui::SliderFloat("Scale", &model->GetTransform().data.scale.x, 0.1f, 10.0f);

				model->GetTransform().data.scale = math::XMFLOAT3(model->GetTransform().data.scale.x, model->GetTransform().data.scale.x, model->GetTransform().data.scale.x);

				ImGui::SliderFloat3("Translate", &model->GetTransform().data.translate.x, -10.0f, 10.0f);
				ImGui::SliderFloat3("Rotate", &model->GetTransform().data.rotation.x, DirectX::XMConvertToRadians(-180.0f), DirectX::XMConvertToRadians(180.0f));

				ImGui::TreePop();
			}
		}

		ImGui::End();
	}

	void Editor::RenderCameraProperties(scene::Camera* camera) const
	{
		ImGui::Begin("Camera Properties");

		if (ImGui::TreeNode("Primary Camera"))
		{
			// Scale uniformally along all axises.
			ImGui::SliderFloat("Movement Speed", &camera->mMovementSpeed, 0.1f, 1000.0f);
			ImGui::SliderFloat("Rotation Speed", &camera->mRotationSpeed, 0.1f, 250.0f);

			ImGui::SliderFloat("Smoothness Factor", &camera->mSmoothnessFactor, 0.0f, 1.0f);

			ImGui::SliderFloat("Movment Multiplier", &camera->mMovementSmoothnessMultipler, 0.0f, 5.0f);
			ImGui::SliderFloat("Rotation Multiplier", &camera->mRotationSmoothnessMultipler, 0.0f, 5.0f);

			ImGui::TreePop();
		}

		ImGui::End();
	}

	void Editor::RenderSceneProperties(std::span<float, 4> clearColor) const
	{
		ImGui::Begin("Scene Properties");
		ImGui::ColorPicker3("Clear Color", clearColor.data(), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
		ImGui::End();
	}

	void Editor::RenderSceneViewport(gfx::DescriptorHandle rtDescriptorHandle) const
	{
		ImGui::Begin("View Port");
		ImGui::Image((ImTextureID)(rtDescriptorHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();
	}

	void Editor::ShowUI(bool value)
	{
		mShowUI = value;
	}

	void Editor::OnResize(Uint2 dimensions) const
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::GetMainViewport()->Size = ImVec2((float)dimensions.x, (float)dimensions.y);
	}
}
