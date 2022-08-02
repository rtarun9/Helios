#pragma once

#include "Pch.hpp"

#include "Graphics/API/Device.hpp"

#include "Scene/Scene.hpp"

namespace helios::editor
{
	// The editor handles all UI related task.
	// The goal is to avoid having UI specific configurations / settings in other abstractions, and have everything contained within this class.
	class Editor
	{
	public:
		Editor(const gfx::Device* device);
		~Editor();

		// Goal is to call this single function from the engine which does all the UI internally, helps make the engine clean as well.
		// This function is heavy WIP and not given as much importance as other abstractions.
		void Render(const gfx::Device* device, scene::Scene* scene, std::span<float, 4> clearColor, PostProcessBuffer& postProcessBufferData, gfx::DescriptorHandle rtDescriptorHandle, gfx::GraphicsContext* graphicsContext);

		void OnResize(Uint2 dimensions) const;

		void ShowUI(bool value);

	private:
		void SetCustomDarkTheme() const;

		void RenderSceneHierarchy(std::span<std::unique_ptr<helios::scene::Model>> models) const;
		void RenderCameraProperties(scene::Camera* camera) const;
		void RendererProperties(std::span<float, 4> clearColor, PostProcessBuffer& postProcessBufferData) const;

		void RenderLightProperties(std::vector<std::unique_ptr<helios::scene::Light>>& lights) const;

		// Accepts the pay load (accepts data which is dragged in from content browser to the scene view port, and loads the model (if path belongs to a .gltf file).
		void RenderSceneViewport(const gfx::Device* device, gfx::DescriptorHandle rtDescriptorHandle, scene::Scene* scene) const;

		// Handle drag and drop of models into viewport at run time.
		void RenderContentBrowser();

	private:
		bool mShowUI{ true };

		// Member variables for content browser.
		static inline const std::filesystem::path ASSETS_PATH = { L"Assets" };

		std::filesystem::path mContentBrowserCurrentPath{ ASSETS_PATH };
	};
}