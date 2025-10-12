#include <stdbool.h>
#include "game.h"
#include "state.h"
#include "save.h"
#include "fade.h"
#include "../states/game/gamestate.h"
#include "../states/menu/menustate.h"

static GameState states[STATE_COUNT];

static enum GameStateId currentStateId;
static enum GameStateId futureStateId = -1;

GameState* currentState = NULL;

void loadStates(void)
{
    states[ST_GAMESTATE].update = &gamestate_update;
    states[ST_GAMESTATE].draw = &gamestate_draw;
    states[ST_GAMESTATE].init = &gamestate_init;
    states[ST_GAMESTATE].stop = &gamestate_stop;
    states[ST_GAMESTATE].controller_pressed = &gamestate_control_pressed;
    states[ST_GAMESTATE].controller_released = &gamestate_control_released;

    states[ST_MENUSTATE].update = &menustate_update;
    states[ST_MENUSTATE].draw = &menustate_draw;
    states[ST_MENUSTATE].init = &menustate_init;
    states[ST_MENUSTATE].stop = &menustate_stop;
    states[ST_MENUSTATE].controller_pressed = &menustate_control_pressed;
    states[ST_MENUSTATE].controller_released = &menustate_control_released;

    currentState = NULL;
}

// Perform the actual state change
static void doStateChange(enum GameStateId newStateId)
{
    saveSettings();
    if(newStateId >= STATE_COUNT || newStateId < 0) //Invalid state Id = quit the game
    {
        exit(0);
        return;
    }
    
    if(currentState && currentState->stop)
        currentState->stop();

    currentStateId = newStateId;
    currentState = &states[newStateId];

    if(currentState->init)
        currentState->init(gameRenderer);
}

void changeStateInstant(enum GameStateId newStateId)
{
    if(fade_isFadeInProgress())
        fade_stopFade();
    
    doStateChange(newStateId);
}

// Change state after a fade out and start a fade in, has to be called as changeState fade end callback
static void changeStateAfterFade(void)
{
    if(futureStateId < 0)
        return;

    doStateChange(futureStateId);
    futureStateId = -1;
    fade_setFadeEndCallback(NULL);
    fade_doFadeIn(0.25f, (SDL_Color){0,0,0,SDL_ALPHA_TRANSPARENT});
}

void changeState(enum GameStateId newStateId)
{
    if(newStateId >= STATE_COUNT || newStateId < 0) //Invalid state Id = quit the game
    {
        saveSettings();
        exit(0);
        return;
    }
    
    futureStateId = newStateId;
    if(fade_isFadeInProgress())
        fade_stopFade();
    fade_setFadeEndCallback(&changeStateAfterFade);
    fade_doFadeOut(0.17f, (SDL_Color){0,0,0,SDL_ALPHA_OPAQUE});
}
