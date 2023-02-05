#pragma once

#include <SDL_scancode.h>

namespace helios::core
{
    // For now, SDL2 is the only platform back end being used.
    // Due to this, the enum class entries are directly mapped to SDL Scan codes.
    // note(rtarun9) : Determine if a hash map based approach for mapping from custom Input enum class
    // to platform backend specific ones is required.

    enum class Keys : uint8_t
    {
        W = SDL_SCANCODE_W,
        A = SDL_SCANCODE_A,
        S = SDL_SCANCODE_S,
        D = SDL_SCANCODE_D,
        AUp = SDL_SCANCODE_UP,
        ALeft = SDL_SCANCODE_LEFT,
        ADown = SDL_SCANCODE_DOWN,
        ARight = SDL_SCANCODE_RIGHT,
    };

    struct Input
    {
        bool getKeyState(const Keys key) const
        {
            return m_keyStates[enumClassValue(key)];
        }

        // Helper function to update the key states array conveniently.
        void processInput(const uint8_t* keyboardState);

        std::array<bool, SDL_NUM_SCANCODES> m_keyStates{};
    };
} // namespace helios::core