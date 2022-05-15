#pragma once

#include "Pch.hpp"

#include "Graphics/API/Descriptor.hpp"

#include "imgui.h"

namespace helios
{
	class UIManager
	{
	public:
		void Init(ID3D12Device* device, uint32_t framesInFlight, gfx::Descriptor& srvDescriptor) const;
		void ShutDown() const;

		void FrameStart() const;
		void FrameEnd(ID3D12GraphicsCommandList* commandList) const;

		void Begin(std::wstring_view uiComponentName) const;
		void End() const;

		void SetClearColor(std::span<float> clearColor) const;
	};
}
