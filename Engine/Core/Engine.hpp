#pragma once

#include "Pch.hpp"

#include "Graphics/API/Device.hpp"

namespace helios
{
	struct Config
	{
		std::wstring title{};
		uint32_t width{};
		uint32_t height{};
	};

	// The Base Engine class is passed into the Application static classe's Run method.
	// All SandBoxes / Test Environment's are to derive from the Engine class.
	class Engine
	{
	public:
		// Number of 32 bit root constants
		static constexpr uint32_t NUMBER_32_BIT_ROOTCONSTANTS = 64u;

		Engine(Config& config);
		virtual ~Engine() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnDestroy() = 0;

		virtual void OnKeyAction(uint8_t keycode, bool isKeyDown);
		virtual void OnResize();

		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; };
		std::wstring GetTitle() const { return mTitle; };

	public:
		uint32_t mWidth{};
		uint32_t mHeight{};
		std::wstring mTitle{};

		float mAspectRatio{};

		uint64_t mFrameIndex{};

		std::unique_ptr<gfx::Device> mDevice{};
	};
}