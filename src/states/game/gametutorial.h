#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

//If true, the tutorial code won't run anymore
extern bool tutorialFinished;

void gametutorial_init(void);

bool gametutorial_update(void);

void gametutorial_draw(SDL_Renderer* renderer);

bool gametutorial_press(SDL_GameControllerButton button);

void gametutorial_stop(void);
