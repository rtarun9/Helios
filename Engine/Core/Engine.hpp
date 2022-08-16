#pragma once

#include "Pch.hpp"

#include "Graphics/API/Device.hpp"

namespace helios::core
{
	struct Config
	{
		std::wstring title{};
		Uint2 dimensions{};
	};

	// The Base Engine class is passed into the Application static classes Run method.
	// All SandBoxes / Test Environment's are to derive from the Engine class.
	class Engine
	{
	public:
		Engine() = delete;
		Engine(Config& config);

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnDestroy() = 0;

		virtual void OnKeyAction(uint8_t keycode, bool isKeyDown) = 0;
		virtual void OnResize() = 0;

		Uint2 GetDimensions() const { return mDimensions; }
		std::wstring GetTitle() const { return mTitle; };

	protected:
		Uint2 mDimensions{};
		std::wstring mTitle{};

		float mAspectRatio{};

		uint64_t mFrameIndex{};

		std::unique_ptr<gfx::Device> mDevice{};
	};
}