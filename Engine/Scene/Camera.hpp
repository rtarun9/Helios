#pragma once

#include "Pch.hpp"

#include "Graphics/API/Resources.hpp"
#include "Graphics/API/Device.hpp"

namespace helios::scene
{
	// WASD used for camera position, Arrow keys used to change camera orientation.
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

	// The camera class will not have a buffer on its own, rather the camera data (position, view matrix, look at direction etc)
	// will be added to the SceneBuffer, which is handled by the Engine.
	class Camera
	{
	public:
		Camera(float movementSpeed = 500.0f, float rotationSpeed = 5.0f);

		void HandleInput(uint8_t keycode, bool isKeyDown);

		void Update(float deltaTime);

		DirectX::XMMATRIX GetViewMatrix() const { return mViewMatrix; }
		DirectX::XMFLOAT3 GetCameraPosition() const { return mCameraPosition; }
		DirectX::XMFLOAT3 GetCameraTarget() const { return mCameraTarget; }

	public:
		// The member variables are made public so the Editor can access it.
		// note(rtarun9) : Usually member variables are stored in XMFloat3 / XmFloat4x4 types, but in some cases when no operations are done on some
		// member variables (i.e constant world orientation, etc), they are directly stored in SIMD compatible types.
		const DirectX::XMVECTOR mWorldFront{ DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };
		const DirectX::XMVECTOR mWorldRight{ DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
		const DirectX::XMVECTOR mWorldUp{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

		DirectX::XMFLOAT3 mCameraFront{ 0.0f, 0.0f, 1.0f};
		DirectX::XMFLOAT3 mCameraRight{ 1.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 mCameraUp{ 0.0f, 1.0f, 0.0f };

		DirectX::XMFLOAT3 mCameraTarget{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 mCameraPosition{ 0.0f, 0.0f, -10.0f };

		DirectX::XMMATRIX mViewMatrix{};

		// Movement speed is for WASD
		// Rotation speed is for orientation
		float mMovementSpeed{ 500.0f };
		float mRotationSpeed{ 5.0f };

		// Used to control how fast or slow to lerp to rest position. For camera rotation / movement, the smoothness factor is multiplied by the corresponding SmoothnessMultiplier.
		float mFrictionFactor{ 0.032f };

		float mYaw{ 0.0f };
		float mPitch{ 0.0f };

		std::array<bool, EnumClassValue(Keys::TotalKeyCount)> mKeyStates{ false };
	};
}