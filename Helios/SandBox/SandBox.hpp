#pragma once

#include "Pch.hpp"

#include "Core/Engine.hpp"

namespace helios
{
	class SandBox : public Engine
	{
	public:
		SandBox(Config& config);

		virtual void OnInit() override;
		virtual void OnUpdate() override;
		virtual void OnRender() override;
		virtual void OnDestroy() override;

		void OnKeyDown(uint32_t keycode);
		void OnKeyUp(uint32_t keycode);
	};
}