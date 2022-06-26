#pragma once

#include "Pch.hpp"

#include "Common/ConstantBuffers.hlsli"

#include "Graphics/API/ConstantBuffer.hpp"
#include "Graphics/API/Descriptor.hpp"

namespace helios
{
	enum class Keys : uint8_t
	{
		W,
		A,
		S,
		D,
		AUp,
		ALeft,
		ADown,
		ARight,
		TotalKeyCount
	};

	static std::map<uint8_t, Keys> INPUT_MAP
	{
		{'W', Keys::W},
		{'A', Keys::A},
		{'S', Keys::S},
		{'D', Keys::D},
		{VK_UP, Keys::AUp},
		{VK_LEFT, Keys::ALeft},
		{VK_DOWN, Keys::ADown},
		{VK_RIGHT, Keys::ARight}
	};

	class Camera
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& cbDescriptor);

		void HandleInput(uint8_t keycode, bool isKeyDown);

		void Update(float deltaTime);

		dx::XMMATRIX GetViewMatrix() const;

		uint32_t GetCameraDataCBufferIndex() const { return m_CameraDataCBuffer.GetBufferIndex(); }

	public:
		dx::XMVECTOR m_WorldFront{ dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };
		dx::XMVECTOR m_WorldRight{ dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
		dx::XMVECTOR m_WorldUp{ dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

		dx::XMVECTOR m_CameraFront{ m_WorldFront };
		dx::XMVECTOR m_CameraRight{ m_WorldRight };
		dx::XMVECTOR m_CameraUp{ m_WorldUp };

		dx::XMVECTOR m_CameraTarget{ dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f) };
		dx::XMVECTOR m_CameraPosition{ dx::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f) };

		dx::XMMATRIX m_CameraRotationMatrix{ dx::XMMatrixIdentity() };

		dx::XMMATRIX m_ViewMatrix{ dx::XMMatrixIdentity() };

		// Movement speed is for WASD
		// Rotation speed is for orientation
		float m_MovementSpeed{ 150.0f };
		float m_RotationSpeed{ 0.05f };

		float m_Yaw{ 0.0f };
		float m_Pitch{ 0.0f };

		std::array<bool, EnumClassValue(Keys::TotalKeyCount)> m_KeyStates{false};

		gfx::ConstantBuffer<CameraData> m_CameraDataCBuffer{};
	};
}
