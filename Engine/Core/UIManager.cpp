#include "Pch.hpp"

#include "UIManager.hpp"
#include "Application.hpp"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace helios
{
	void UIManager::Init(ID3D12Device* device, uint32_t framesInFlight, gfx::Descriptor& srvDescriptor) const
	{
		// Setup ImGui context.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;

		// Setup Style.
		ImGui::StyleColorsDark();

		// Setup platform / renderer backends.
		ImGui_ImplWin32_EnableDpiAwareness();

		ImGui_ImplWin32_Init(Application::GetWindowHandle());

		ImGui_ImplDX12_Init(device, framesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM, srvDescriptor.GetDescriptorHeap(), srvDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle, srvDescriptor.GetCurrentDescriptorHandle().gpuDescriptorHandle);

		srvDescriptor.OffsetCurrentHandle();
	}

	void UIManager::FrameStart() const
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void UIManager::ShutDown() const
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void UIManager::FrameEnd(ID3D12GraphicsCommandList* commandList) const
	{
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	void UIManager::Begin(std::wstring_view uiComponentName) const
	{
		ImGui::Begin(WstringToString(uiComponentName).c_str());
	}

	void UIManager::End() const
	{
		ImGui::End();
	}

	void UIManager::SetClearColor(std::span<float> clearColor) const
	{
		if (ImGui::TreeNode("Clear Color"))
		{
			ImGui::ColorEdit3("Clear Color", clearColor.data());
			ImGui::TreePop();
		}
	}
}
