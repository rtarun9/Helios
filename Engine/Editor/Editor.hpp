#pragma once

#include "Pch.hpp"

#include "Graphics/API/Device.hpp"

#include "Scene/Camera.hpp"
#include "Scene/Model.hpp"

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
		void Render(std::span<std::unique_ptr<helios::scene::Model>> models, scene::Camera* camera, std::span<float, 4> clearColor, gfx::DescriptorHandle rtDescriptorHandle, gfx::GraphicsContext* graphicsContext) const;

		void OnResize(Uint2 dimensions) const;

		void ShowUI(bool value);

	private:
		void SetCustomDarkTheme() const;
		
		void RenderModelProperties(std::span<std::unique_ptr<helios::scene::Model>> models) const;
		void RenderCameraProperties(scene::Camera* camera) const;
		void RenderSceneProperties(std::span<float, 4> clearColor) const;
		void RenderSceneViewport(gfx::DescriptorHandle rtDescriptorHandle) const;

	private:
		bool mShowUI{true};
	};
}