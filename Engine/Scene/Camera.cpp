#include "Pch.hpp"

#include "Camera.hpp"

// Some of the operator overloads are in this namespace, hence needed to include the namespace here.
using namespace DirectX;

namespace helios::scene
{
	Camera::Camera(float movementSpeed, float rotationSpeed)
		: mMovementSpeed(movementSpeed), mRotationSpeed(rotationSpeed)
	{
	}

	void Camera::HandleInput(uint8_t keycode, bool isKeyDown)
	{
		if (size_t keycodeValue = EnumClassValue(INPUT_MAP[keycode]); INPUT_MAP.find(keycode) != end(INPUT_MAP))
		{
			mKeyStates[keycodeValue] = isKeyDown;
		}
	}

	void Camera::Update(float deltaTime)
	{
		// Logic for making camera movement smooth.
		// pitchTo, yawTo, cameraMoveToPosition are all depending on only the current frame, and values are set based on user input.
		// The static variables (yawShift, pitchShift, moveTo) etc are persistent across frames.
		// Thier values are lerped over frames. If no input given (for the specific movement), the values are slowly lerped to 0.0f / XMVECTOR(0.0f, 0.0f, 0.0f, 0.0f) or the current frame value (i.e the movements are not instantaneous).
		// The smoothness factor and multipler determine how "smooth" or how slowly you want these values to lerp to 0.0f or current frame value.
		// Reference (WickedEngine) : https://github.com/turanszkij/WickedEngine/commit/42d7592444ff74180f8b48d14a05e947b37fd387.

		float movementSpeed = deltaTime * mMovementSpeed;
		if (!mKeyStates[EnumClassValue(Keys::W)] || !mKeyStates[EnumClassValue(Keys::A)] || !mKeyStates[EnumClassValue(Keys::S)] || !mKeyStates[EnumClassValue(Keys::D)])
		{
			movementSpeed /= 10.0f;
		}

		float rotationSpeed = deltaTime * mRotationSpeed;
		if (!mKeyStates[EnumClassValue(Keys::ALeft)] || !mKeyStates[EnumClassValue(Keys::ARight)] || !mKeyStates[EnumClassValue(Keys::AUp)] || !mKeyStates[EnumClassValue(Keys::ADown)])
		{
			rotationSpeed /= 2.0f;
		}

		math::XMVECTOR cameraFront = math::XMLoadFloat3(&mCameraFront);
		DirectX::XMVECTOR cameraRight = math::XMLoadFloat3(&mCameraRight);

		math::XMVECTOR cameraPosition = math::XMLoadFloat3(&mCameraPosition);
		math::XMVECTOR cameraMoveToPosition = math::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

		static math::XMVECTOR moveTo = math::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		static float yawShift = 0.0f;
		static float pitchShift = 0.0f;
		
		float pitchTo = 0.0f;
		float yawTo = 0.0f;

		if (mKeyStates[EnumClassValue(Keys::W)])
		{
			cameraMoveToPosition += cameraFront;
		}
		else if (mKeyStates[EnumClassValue(Keys::S)])
		{
			cameraMoveToPosition -= cameraFront;
		}

		if (mKeyStates[EnumClassValue(Keys::A)])
		{
			cameraMoveToPosition -= cameraRight;
		}
		else if (mKeyStates[EnumClassValue(Keys::D)])
		{
			cameraMoveToPosition += cameraRight;
		}

		if (mKeyStates[EnumClassValue(Keys::AUp)])
		{
			pitchTo -= rotationSpeed;
		}
		else if (mKeyStates[EnumClassValue(Keys::ADown)])
		{
			pitchTo += rotationSpeed;
		}

		if (mKeyStates[EnumClassValue(Keys::ALeft)])
		{
			yawTo -= rotationSpeed;
		}
		else if (mKeyStates[EnumClassValue(Keys::ARight)])
		{
			yawTo += rotationSpeed;
		}


		cameraMoveToPosition = math::XMVector3Normalize(cameraMoveToPosition) * movementSpeed;
		moveTo = math::XMVectorLerp(moveTo, cameraMoveToPosition, mSmoothnessFactor * mMovementSmoothnessMultipler);

		cameraPosition += moveTo;

		pitchShift = std::lerp(pitchShift, pitchTo, mSmoothnessFactor * mRotationSmoothnessMultipler);
		yawShift = std::lerp(yawShift, yawTo, mSmoothnessFactor * mRotationSmoothnessMultipler);

		mPitch += pitchShift;
		mYaw += yawShift;

		DirectX::XMMATRIX cameraRotationMatrix = math::XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);

		DirectX::XMVECTOR cameraTarget = math::XMLoadFloat3(&mCameraTarget);
		DirectX::XMVECTOR cameraUp = math::XMLoadFloat3(&mCameraUp);

		cameraTarget = math::XMVector3TransformCoord(mWorldFront, cameraRotationMatrix);
		cameraTarget = math::XMVector3Normalize(cameraTarget);

		cameraRight = math::XMVector3TransformCoord(mWorldRight, cameraRotationMatrix);
		cameraFront = math::XMVector3TransformCoord(mWorldFront, cameraRotationMatrix);
		cameraUp = math::XMVector3Cross(cameraFront, cameraRight);

		cameraTarget += cameraPosition;

		mViewMatrix = math::XMMatrixLookAtLH(cameraPosition, cameraTarget, cameraUp);

		// Store all data back into member variables.
		math::XMStoreFloat3(&mCameraFront, cameraFront);
		math::XMStoreFloat3(&mCameraPosition, cameraPosition);
		math::XMStoreFloat3(&mCameraTarget, cameraTarget);
		math::XMStoreFloat3(&mCameraUp, cameraUp);
		math::XMStoreFloat3(&mCameraRight, cameraRight);
	}

	math::XMMATRIX Camera::GetViewMatrix() const
	{
		return mViewMatrix;
	}
}