#pragma once
#include <SDL2/SDL.h>
#include "../../utils/timer.h"

extern KTimer* gameTimer;

void gamestate_init(SDL_Renderer* rend);

void gamestate_update(float dt);

void gamestate_draw(SDL_Renderer* rend);

void gamestate_control_pressed(SDL_GameControllerButton button, const SDL_Event* event);

void gamestate_control_released(SDL_GameControllerButton button, const SDL_Event* event);

void gamestate_stop(void);
