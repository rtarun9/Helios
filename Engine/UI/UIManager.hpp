#pragma once

#include "Pch.hpp"

#include "Graphics/API/Descriptor.hpp"
#include "Graphics/API/Device.hpp"
#include "Graphics/API/GraphicsContext.hpp"

namespace helios::ui
{
	// Thin abstraction over ImGui.
	// Not a part of the device abstraction : so must be created and handled manually in the SandBox files.
	class UIManager
	{
	public:
		UIManager(gfx::Device* device);
		~UIManager();

		void BeginFrame() const;
		void Render(gfx::GraphicsContext* graphicsContext) const;
		void EndFrame() const;

		void BeginPanel(std::wstring_view uiComponentName) const;
		void EndPanel() const;

		void SetClearColor(std::span<float> clearColor) const;

		void SetCustomDarkTheme() const;

		void SliderFloat(std::wstring_view name, float& data, float minExtent = -10.0f, float maxExtent = 10.0f) const;
		void SliderFloat3(std::wstring_view name, float& data, float minExtent = -10.0f, float maxExtent = 10.0f) const;

		// Return true if the node is open (i.e to be shown) or false otherwise.
		bool TreeNode(std::wstring_view name) const;
		void TreePop() const;

		void ShowUI();
		void HideUI();

		void UpdateDisplaySize(Uint2 displaySize);

	private:
		bool mShowUI{ true };
	};
}
