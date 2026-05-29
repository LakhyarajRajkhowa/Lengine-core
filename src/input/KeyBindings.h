#pragma once
#include <SDL2/SDL.h>

namespace EditorKeys
{
    inline constexpr SDL_Keycode MovePosY = SDLK_SPACE;
    inline constexpr SDL_Keycode MoveNegY = SDLK_LSHIFT;
    inline constexpr SDL_Keycode MovePosX = SDLK_d;
    inline constexpr SDL_Keycode MoveNegX = SDLK_a;
    inline constexpr SDL_Keycode MovePosZ = SDLK_w;
    inline constexpr SDL_Keycode MoveNegZ = SDLK_s;


    inline constexpr SDL_Keycode FastMove = SDLK_LCTRL;
    inline constexpr SDL_Keycode Delete = SDLK_x;

    inline constexpr SDL_Keycode All[] = {
        MovePosY,
        MoveNegY,
        MovePosX,
        MoveNegX,
        MovePosZ,
        MoveNegZ,
        FastMove,
        Delete
    };
}

struct GameKeys
{
    // Movement
    SDL_Keycode moveForward = SDLK_w;
    SDL_Keycode moveBackward = SDLK_s;
    SDL_Keycode moveLeft = SDLK_a;
    SDL_Keycode moveRight = SDLK_d;

    // Actions
    SDL_Keycode jump = SDLK_SPACE;
    SDL_Keycode interact = SDLK_e;
    SDL_Keycode pause = SDLK_ESCAPE;
    SDL_Keycode sprint = SDLK_LSHIFT;

    // Mouse buttons
    Uint8 attack = SDL_BUTTON_LEFT;
};

