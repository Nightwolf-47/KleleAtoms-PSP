#pragma once
#include <SDL2/SDL.h>

typedef struct GameState {
    void (*update)(float dt);
    void (*draw)(SDL_Renderer* rend);
    void (*controller_pressed)(SDL_GameControllerButton button, const SDL_Event* event);
    void (*controller_released)(SDL_GameControllerButton button, const SDL_Event* event);
    void (*init)(SDL_Renderer* rend);
    void (*stop)(void);
} GameState;

enum GameStateId {
    ST_GAMESTATE,
    ST_MENUSTATE,

    STATE_COUNT
};

extern GameState* currentState;

// Assign all the state function pointers to states
void loadStates(void);

/// @brief Change game state instantly
/// @param newState GameStateId enum for the new state
void changeStateInstant(enum GameStateId newStateId);

/// @brief Change game state with a fade transition
/// @param newState GameStateId enum for the new state
void changeState(enum GameStateId newState);
