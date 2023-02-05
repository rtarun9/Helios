#pragma once

#include "Core/Input.hpp"

namespace helios::core
{
    void Input::processInput(const uint8_t* keyboardState)
    {
        if (keyboardState[SDL_SCANCODE_W])
        {
            m_keyStates[enumClassValue(Keys::W)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::W)] = false;
        }

        if (keyboardState[SDL_SCANCODE_A])
        {
            m_keyStates[enumClassValue(Keys::A)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::A)] = false;
        }

        if (keyboardState[SDL_SCANCODE_S])
        {
            m_keyStates[enumClassValue(Keys::S)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::S)] = false;
        }

        if (keyboardState[SDL_SCANCODE_D])
        {
            m_keyStates[enumClassValue(Keys::D)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::D)] = false;
        }

        if (keyboardState[SDL_SCANCODE_UP])
        {
            m_keyStates[enumClassValue(Keys::AUp)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::AUp)] = false;
        }

        if (keyboardState[SDL_SCANCODE_DOWN])
        {
            m_keyStates[enumClassValue(Keys::ADown)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::ADown)] = false;
        }

        if (keyboardState[SDL_SCANCODE_LEFT])
        {
            m_keyStates[enumClassValue(Keys::ALeft)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::ALeft)] = false;
        }

        if (keyboardState[SDL_SCANCODE_RIGHT])
        {
            m_keyStates[enumClassValue(Keys::ARight)] = true;
        }
        else
        {
            m_keyStates[enumClassValue(Keys::ARight)] = false;
        }
    }

}