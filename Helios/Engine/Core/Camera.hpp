#pragma once

#include "Pch.hpp"

namespace helios
{
	class Camera
	{
	public:
		void HandleInput(uint8_t keycode, bool isKeyDown);

		void Update(float deltaTime);

		dx::XMMATRIX GetViewMatrix() const;

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
		float m_MovementSpeed{ 50.0f };
		float m_RotationSpeed{ 0.05f };

		float m_Yaw{ 0.0f };
		float m_Pitch{ 0.0f };

		std::array<bool, EnumClassValue(Keys::TotalKeyCount)> m_KeyStates{false};
	};
}
