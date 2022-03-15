#include "Pch.hpp"

#include "Camera.hpp"

// Some of the operator overloads are in this namespace, hence needed to include the namespace here.
using namespace DirectX;

namespace helios
{
	void Camera::HandleInput(uint8_t keycode, bool isKeyDown)
	{
		if (INPUT_MAP.find(keycode) != end(INPUT_MAP))
		{
			m_KeyStates[EnumClassValue(INPUT_MAP[keycode])] = isKeyDown;
		}
	}

	void Camera::Update(float deltaTime)
	{
		float movementSpeed = deltaTime * m_MovementSpeed / 10.0f;

		if (m_KeyStates[EnumClassValue(Keys::W)])
		{
			m_CameraPosition += m_CameraFront * movementSpeed;
		}
		else if (m_KeyStates[EnumClassValue(Keys::S)])
		{
			m_CameraPosition -= m_CameraFront * movementSpeed;
		}

		if (m_KeyStates[EnumClassValue(Keys::A)])
		{
			m_CameraPosition -= m_CameraRight * movementSpeed;
		}
		else if (m_KeyStates[EnumClassValue(Keys::D)])
		{
			m_CameraPosition += m_CameraRight * movementSpeed;
		}

		if (m_KeyStates[EnumClassValue(Keys::AUp)])
		{
			m_Pitch -= m_RotationSpeed;
		}
		else if (m_KeyStates[EnumClassValue(Keys::ADown)])
		{
			m_Pitch += m_RotationSpeed;
		}

		if (m_KeyStates[EnumClassValue(Keys::ALeft)])
		{
			m_Yaw -= m_RotationSpeed;
		}
		else if (m_KeyStates[EnumClassValue(Keys::ARight)])
		{
			m_Yaw += m_RotationSpeed;
		}

		m_CameraRotationMatrix = dx::XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0.0f);

		m_CameraTarget = dx::XMVector3TransformCoord(m_WorldFront, m_CameraRotationMatrix);
		m_CameraTarget = dx::XMVector3Normalize(m_CameraTarget);

		m_CameraRight = dx::XMVector3TransformCoord(m_WorldRight, m_CameraRotationMatrix);
		m_CameraFront = dx::XMVector3TransformCoord(m_WorldFront, m_CameraRotationMatrix);
		m_CameraUp = dx::XMVector3Cross(m_CameraFront, m_CameraRight);

		m_CameraTarget = m_CameraPosition + m_CameraTarget;

		m_ViewMatrix = dx::XMMatrixLookAtLH(m_CameraPosition, m_CameraTarget, m_CameraUp);
	}

	dx::XMMATRIX Camera::GetViewMatrix() const
	{
		return m_ViewMatrix;
	}
}
