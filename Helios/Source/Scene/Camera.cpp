#include "Pch.hpp"

#include "Scene/Camera.hpp"

using namespace math;

namespace helios::scene
{
    void Camera::update(const float deltaTime, const core::Input& input)
    {
        // Logic for making camera movement smooth.
        // pitchTo, yawTo, cameraMoveToPosition are all depending on only the current frame, and values are set based on
        // user input at that current instance of time. The static variables (yawShift, pitchShift, moveTo) etc are
        // persistent across frames, hence made static.

        // Their values (of the persistent static variables) are linearly interpolated over frames. If no input given
        // (for the specific movement), the values are slowly interpolated to
        // the current frame value (i.e the movements are not instantaneous). By this logic, the values will be
        // interpolated to 0 when no input is given. Hence, this case (of no input) need not be separately handled.
        // In short, we want our camera position and euler angle orientation values to lerp between frames.

        // The friction "smooth" or how slowly / fast
        //  Reference (WickedEngine) :
        //  https://github.com/turanszkij/WickedEngine/commit/42d7592444ff74180f8b48d14a05e947b37fd387.

        const float movementSpeed = m_movementSpeed * deltaTime;
        const float rotationSpeed = m_rotationSpeed * deltaTime;

        const math::XMVECTOR cameraFront = math::XMLoadFloat4(&m_cameraFront);
        const math::XMVECTOR cameraRight = math::XMLoadFloat4(&m_cameraRight);

        math::XMVECTOR cameraPosition = math::XMLoadFloat4(&m_cameraPosition);

        math::XMVECTOR cameraMoveToPosition = math::XMVECTOR{0.0f, 0.0f, 0.0f, 0.0f};

        static math::XMVECTOR moveTo = math::XMVECTOR{0.0f, 0.0f, 0.0f, 0.0f};
        static float yawShift = 0.0f;
        static float pitchShift = 0.0f;

        float pitchTo = 0.0f;
        float yawTo = 0.0f;

        // Handle input.
        if (input.getKeyState(core::Keys::W))
        {
            cameraMoveToPosition += cameraFront * movementSpeed;
        }
        else if (input.getKeyState(core::Keys::S))
        {
            cameraMoveToPosition -= cameraFront * movementSpeed;
        }

        if (input.getKeyState(core::Keys::A))
        {
            cameraMoveToPosition -= cameraRight * movementSpeed;
        }
        else if (input.getKeyState(core::Keys::D))
        {
            cameraMoveToPosition += cameraRight * movementSpeed;
        }

        if (input.getKeyState(core::Keys::AUp))
        {
            pitchTo -= rotationSpeed;
        }
        else if (input.getKeyState(core::Keys::ADown))
        {
            pitchTo += rotationSpeed;
        }

        if (input.getKeyState(core::Keys::ALeft))
        {
            yawTo -= rotationSpeed;
        }
        else if (input.getKeyState(core::Keys::ARight))
        {
            yawTo += rotationSpeed;
        }

        // note(rtarun9) : movementSpeed is multiplied again since while normalizing the vector, we loose that
        // information. consider removing this multiplication from the code above where cameraMoveToPosition is set.
        cameraMoveToPosition = math::XMVector3Normalize(cameraMoveToPosition) * movementSpeed;
        moveTo = math::XMVectorLerp(moveTo, cameraMoveToPosition, m_frictionFactor);

        cameraPosition += moveTo;

        pitchShift = std::lerp(pitchShift, pitchTo, m_frictionFactor);
        yawShift = std::lerp(yawShift, yawTo, m_frictionFactor);

        m_pitch += pitchShift;
        m_yaw += yawShift;

        math::XMStoreFloat4(&m_cameraPosition, cameraPosition);
    }

    math::XMMATRIX Camera::computeAndGetViewMatrix()
    {
        // Load all XMFLOATX into XMVECTOR's.
        // The target is camera position + camera front direction (i.e direction it is looking at).

        const math::XMMATRIX rotationMatrix = math::XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

        static constexpr math::XMVECTOR worldUp = math::XMVECTOR{0.0f, 1.0f, 0.0f, 0.0f};
        static constexpr math::XMVECTOR worldRight = math::XMVECTOR{1.0f, 0.0f, 0.0f, 0.0f};
        static constexpr math::XMVECTOR worldFront = math::XMVECTOR{0.0f, 0.0f, 1.0f, 0.0f};

        const math::XMVECTOR cameraRight =
            math::XMVector3Normalize(math::XMVector3TransformCoord(worldRight, rotationMatrix));

        const math::XMVECTOR cameraFront =
            math::XMVector3Normalize(math::XMVector3TransformCoord(worldFront, rotationMatrix));

        const math::XMVECTOR cameraUp = math::XMVector3Normalize(math::XMVector3Cross(cameraFront, cameraRight));

        const math::XMVECTOR cameraPosition = math::XMLoadFloat4(&m_cameraPosition);

        const math::XMVECTOR cameraTarget = cameraPosition + cameraFront;

        math::XMStoreFloat4(&m_cameraRight, cameraRight);
        math::XMStoreFloat4(&m_cameraFront, cameraFront);
        math::XMStoreFloat4(&m_cameraUp, cameraUp);

        return math::XMMatrixLookAtLH(cameraPosition, cameraTarget, cameraUp);
    }
} // namespace helios::scene
