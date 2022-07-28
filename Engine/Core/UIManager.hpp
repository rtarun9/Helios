#pragma once

#include "Pch.hpp"

#include "Graphics/API/Descriptor.hpp"
#include "Graphics/API/Device.hpp"
#include "Graphics/API/GraphicsContext.hpp"

#include "imgui.h"

namespace helios
{
	// Thin abstraction over ImGui.
	// Not a part of the device abstraction : so must be created and handled manually in the SandBox files.
	class UIManager
	{
	public:
		UIManager(gfx::Device* device);
		void ShutDown() const;

		void BeginFrame() const;
		void EndFrame(gfx::GraphicsContext* graphicsContext) const;

		void Begin(std::wstring_view uiComponentName) const;
		void End() const;

		void SetClearColor(std::span<float> clearColor) const;

		void SetCustomDarkTheme() const;

		void ShowUI();
		void HideUI();

		void UpdateDisplaySize(Uint2 displaySize);

	private:
		bool mShowUI{ true };
	};
}
