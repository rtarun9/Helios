#pragma once

#include "Pch.hpp"

#include "Graphics/Descriptor.hpp"

#include "imgui.h"

namespace helios
{
	class UIManager
	{
	public:
		void Init(ID3D12Device* device, uint32_t framesInFlight, gfx::Descriptor& srvDescriptor);
		void ShutDown();

		void FrameStart();
		void FrameEnd(ID3D12GraphicsCommandList* commandList);

		void Begin();
		void End();

		void SetClearColor(std::span<float> clearColor);
	};
}
