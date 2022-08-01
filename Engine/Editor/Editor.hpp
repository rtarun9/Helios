#pragma once

#include "Pch.hpp"

#include "Graphics/API/Device.hpp"

#include "Scene/Camera.hpp"
#include "Scene/Model.hpp"
#include "Scene/Light.hpp"

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
		// When ECS is setup, it should be enough to pass a single object to this function.
		// This function is heavy WIP and not given as much importance as other abstractions.
		void Render(const gfx::Device* device, std::vector<std::unique_ptr<helios::scene::Model>>& models, std::vector<std::unique_ptr<helios::scene::Light>>& lights, scene::Camera* camera, std::span<float, 4> clearColor, float& exposure, gfx::DescriptorHandle rtDescriptorHandle, gfx::GraphicsContext* graphicsContext);

		void OnResize(Uint2 dimensions) const;

		void ShowUI(bool value);

	private:
		void SetCustomDarkTheme() const;

		void RenderSceneHierarchy(std::span<std::unique_ptr<helios::scene::Model>> models) const;
		void RenderCameraProperties(scene::Camera* camera) const;
		void RenderSceneProperties(std::span<float, 4> clearColor, float& exposure) const;

		void RenderLightProperties(std::vector<std::unique_ptr<helios::scene::Light>>& lights) const;

		// Accepts the pay load (accepts data which is dragged in from content browser to the scene view port, and loads the model (if path belongs to a .gltf file).
		void RenderSceneViewport(const gfx::Device* device, gfx::DescriptorHandle rtDescriptorHandle, std::vector<std::unique_ptr<helios::scene::Model>>& models) const;

		// Handle drag and drop of models into viewport at run time.
		void RenderContentBrowser();

	private:
		bool mShowUI{ true };

		// Member variables for content browser.
		static inline const std::filesystem::path ASSETS_PATH = { L"Assets" };

		std::filesystem::path mContentBrowserCurrentPath{ ASSETS_PATH };
	};
}