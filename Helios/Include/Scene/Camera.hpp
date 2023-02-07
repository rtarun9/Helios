#pragma once

#include "Core/Input.hpp"

namespace helios::scene
{
    // Provides functions to compute the view matrix that can be used to transform world space coordinates into view
    // space coordinates.
    class Camera
    {
      public:
        // note(rtarun9) : Not defining any special functions (constructor, destructor, etc), since it is not really
        // required here.

        // Update the camera position and orientation values (i.e the Euler angles pitch and yaw).
        void update(const float deltaTime, const core::Input& input);

        [[nodiscard]] math::XMMATRIX computeAndGetViewMatrix();

      public:
        math::XMFLOAT4 m_cameraPosition{0.0f, 0.0f, -5.0f, 1.0f};

        math::XMFLOAT4 m_cameraFront{0.0f, 0.0f, 1.0f, 0.0f};
        math::XMFLOAT4 m_cameraRight{1.0f, 0.0f, 0.0f, 0.0f};
        math::XMFLOAT4 m_cameraUp{0.0f, 1.0f, 0.0f, 0.0f};

        // Euler angle for x axis.
        float m_pitch{};

        // Euler angle for y axis.
        float m_yaw{};

        float m_movementSpeed{0.01f};
        float m_rotationSpeed{0.0015f};

        // Used to control how fast or slow to lerp to rest position.
        float m_frictionFactor{0.32f};
    };
} // namespace helios::scene    