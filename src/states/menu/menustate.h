#pragma once
#include <SDL2/SDL.h>

void menustate_init(SDL_Renderer* rend);

void menustate_update(float dt);

void menustate_draw(SDL_Renderer* rend);

void menustate_control_pressed(SDL_GameControllerButton button, const SDL_Event* event);

void menustate_control_released(SDL_GameControllerButton button, const SDL_Event* event);

void menustate_stop(void);
