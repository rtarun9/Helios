#include "Pch.hpp"

#include "UIManager.hpp"
#include "Core/Application.hpp"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// note(rtarun9) : TODO : ImGui Docking and viewport support.

namespace helios::ui
{
	UIManager::UIManager(gfx::Device* device)
	{
		ImGui_ImplWin32_EnableDpiAwareness();

		// Setup ImGui context.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Reference : https://github.com/ocornut/imgui/blob/docking/examples/example_win32_directx12/main.cpp

		io.DisplaySize = ImVec2((float)core::Application::GetClientDimensions().x, (float)core::Application::GetClientDimensions().y);
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		//ImGuiStyle& style = ImGui::GetStyle();
		//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		//{
		//	style.WindowRounding = 0.0f;
		//	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		//}

		SetCustomDarkTheme();
		// Setup platform / renderer backends.

		ImGui_ImplWin32_Init(core::Application::GetWindowHandle());
		gfx::DescriptorHandle srvDescriptorHandle = device->GetSrvCbvUavDescriptor()->GetCurrentDescriptorHandle();

		ImGui_ImplDX12_Init(device->GetDevice(), gfx::Device::NUMBER_OF_FRAMES, DXGI_FORMAT_R8G8B8A8_UNORM, device->GetSrvCbvUavDescriptor()->GetDescriptorHeap(), srvDescriptorHandle.cpuDescriptorHandle, srvDescriptorHandle.gpuDescriptorHandle);
		device->GetSrvCbvUavDescriptor()->OffsetCurrentHandle();
	}

	void UIManager::BeginFrame() const
	{
		if (mShowUI)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			
			ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		}
	}

	UIManager::~UIManager()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void UIManager::Render(gfx::GraphicsContext* commandList) const
	{
		if (mShowUI)
		{
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetCommandList());
		}
	}

	void UIManager::BeginPanel(std::wstring_view uiComponentName) const
	{
		if (mShowUI)
		{
			ImGui::Begin(WstringToString(uiComponentName).c_str());
		}
	}

	void UIManager::EndPanel() const
	{
		ImGui::End();
	}

	void UIManager::EndFrame() const
	{
		if (mShowUI)
		{
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			//{
				//ImGui::UpdatePlatformWindows();
				//ImGui::RenderPlatformWindowsDefault(NULL, (void*)nullptr);
			//}
		}
	}

	void UIManager::SetClearColor(std::span<float> clearColor) const
	{
		if (mShowUI)
		{
			ImGui::ColorEdit3("Clear Color", clearColor.data());
		}
	}

	void UIManager::SetCustomDarkTheme() const
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

	void UIManager::SliderFloat(std::wstring_view name, float& data, float minExtent, float maxExtent) const
	{
		if (mShowUI)
		{
			ImGui::SliderFloat(WstringToString(name).c_str(), &data, minExtent, maxExtent);
		}
	}

	void UIManager::SliderFloat3(std::wstring_view name, float& data, float minExtent, float maxExtent) const
	{
		if (mShowUI)
		{
			ImGui::SliderFloat3(WstringToString(name).c_str(), &data, minExtent, maxExtent);
		}
	}

	bool UIManager::TreeNode(std::wstring_view name) const
	{
		if (mShowUI)
		{
			return ImGui::TreeNode(WstringToString(name).c_str());
		}

		return false;
	}

	void UIManager::TreePop() const
	{
		if (mShowUI)
		{
			ImGui::TreePop();
		}
	}

	void UIManager::ShowUI()
	{
		mShowUI = true;
	}

	void UIManager::HideUI()
	{
		mShowUI = false;
	}

	void UIManager::UpdateDisplaySize(Uint2 displaySize)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::GetMainViewport()->Size = ImVec2((float)displaySize.x, (float)displaySize.y);
	}
}
