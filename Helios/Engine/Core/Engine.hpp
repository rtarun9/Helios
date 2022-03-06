#pragma once

#include "Pch.hpp"

namespace helios
{
	struct Config
	{
		std::wstring title{};
		uint32_t width{};
		uint32_t height{};
	};

	class Engine
	{
	public:
		Engine(Config& config);
		virtual ~Engine() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnDestroy() = 0;

		virtual void OnKeyAction(uint8_t keycode, bool isKeyDown);
		virtual void OnResize();

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		std::wstring GetTitle() const;

	protected:
		uint32_t m_Width{};
		uint32_t m_Height{};
		std::wstring m_Title{};

		float m_AspectRatio{};

		uint64_t m_FrameIndex{};
	};
}