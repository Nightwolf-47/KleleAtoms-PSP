#include "gamestate.h"
#include "gamelogic.h"
#include "gamedraw.h"
#include "gametutorial.h"
#include "../../game/save.h"
#include "../../game/game.h"
#include "../../game/state.h"

struct ContinuousMoveDir {
    bool moving;
    KTimer* moveTimer;
    int movex;
    int movey;
};

// Selected tile position
static Vec2 selectedTile;

//Structure holding data about continuous tile selector movement
static struct ContinuousMoveDir moveDir;

//Timer for current game time
KTimer* gameTimer;

static bool gamePaused = false; // If true, the game is paused
static bool gamePausing = false; // If true, the game is going to pause when all animations and explosions stop
static Sint64 pausedMillis; // Game time at the time of pause

/// @brief Moves the tile selector position and wraps the positions around if they go out of bounds
/// @param x X selector movement
/// @param y Y selector movement
static void moveSelector(int x, int y)
{
    selectedTile.x = (selectedTile.x + logicData->gridWidth + x) % logicData->gridWidth;
    selectedTile.y = (selectedTile.y + logicData->gridHeight + y) % logicData->gridHeight;
}

// Checks if selector is being moved
static void checkSelectorMovement(void)
{
    moveDir.movex = SDL_clamp(moveDir.movex, -1, 1);
    moveDir.movey = SDL_clamp(moveDir.movey, -1, 1);

    if(moveDir.movex == 0 && moveDir.movey == 0)
    {
        ktimer_destroy(moveDir.moveTimer);
        moveDir.moveTimer = NULL;
        moveDir.moving = false;
    } 
}

/// @brief Move the selector in given position and setup continuous movement
/// @param x X selector movement
/// @param y Y selector movement
static void doSelectorMovement(int x, int y)
{
    moveSelector(x,y);
    if(!moveDir.moving)
    {
        moveDir.moving = true;
        moveDir.moveTimer = ktimer_create();
        ktimer_setTimeMillis(moveDir.moveTimer,-100);
        moveDir.movex = x;
        moveDir.movey = y;
    }
    else
    {
        ktimer_setTimeMillis(moveDir.moveTimer,0);
        moveDir.movex += x;
        moveDir.movey += y;
    }
    checkSelectorMovement();
}

void gamestate_init(SDL_Renderer* rend)
{
    tutorialFinished = true;
    pausedMillis = -1;
    gamePaused = false;
    gameTimer = ktimer_create();
    selectedTile = (Vec2){0,0};
    int ttime = -1;
    if(launchedTutorial)
    {
        int tutPlayerTypes[4] = {1,1,1,1};
        gamelogic_init(10, 6, &tutPlayerTypes);
        gametutorial_init();
    }
    else
    {
        int playerTypes[4] = {gameSettings.player1Type, gameSettings.player2Type, gameSettings.player3Type, gameSettings.player4Type};
        gamelogic_init(gameSettings.gridWidth, gameSettings.gridHeight, &playerTypes);
        ttime = loadGame();
    }
    gamedraw_initAssets(logicData->gridWidth, logicData->gridHeight);
    if(ttime != -1)
    {
        ktimer_setTimeSeconds(gameTimer, ttime);
        game_printMsg("Saved game loaded.",3);
    }
}

void gamestate_update(float dt)
{
    if(logicData->totalPlayerCount < 2)
    {
        changeState(ST_MENUSTATE);
        return;
    }

    if(logicData->playerWon != NOPLAYER || gamePaused)
    {
        return;
    }
    else if(gamePausing)
    {
        if(logicData->atomStackPos==0 && !logicData->animPlaying)
        {
            game_printMsg("",0);
            gamePausing = false;
            pausedMillis = ktimer_getTimeMillis(gameTimer);
            gamePaused = true;
            return;
        }
        else
        {
            game_printMsg("Pausing...",1);
        }
    }

    if(pausedMillis >= 0)
    {
        ktimer_setTimeMillis(gameTimer,pausedMillis);
        pausedMillis = -1;
    }

    if(gametutorial_update())
        return;
    
    gamelogic_tick(dt);
    if(moveDir.moving && moveDir.moveTimer && ktimer_getTimeMillis(moveDir.moveTimer) >= 150)
    {
        moveSelector(moveDir.movex, moveDir.movey);
        ktimer_setTimeMillis(moveDir.moveTimer,0);
    }
}

void gamestate_draw(SDL_Renderer* rend)
{
    Sint64 seconds = (pausedMillis < 0) ? ktimer_getTimeSeconds(gameTimer) : (pausedMillis / 1000);
    gamedraw_drawHUD(seconds);
    gamedraw_drawGrid();
    gamedraw_drawAtoms();
    if(logicData->playerWon != NOPLAYER)
        gamedraw_drawVictoryWindow(seconds);
    else if(!gamePaused && tutorialFinished)
        gamedraw_drawSelector(selectedTile.x, selectedTile.y);
    
    if(launchedTutorial)
        gametutorial_draw(rend);

    if(gamePaused)
        gamedraw_drawPauseWindow();
}

void gamestate_control_pressed(SDL_GameControllerButton button, const SDL_Event *event)
{
    if(launchedTutorial && gametutorial_press(button))
        return;

    if(logicData->playerWon != NOPLAYER)
    {
        changeState(ST_MENUSTATE);
        return;
    }

    if(gamePaused)
    {
        switch(button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                if(pausedMillis >= 0)
                    ktimer_setTimeMillis(gameTimer, pausedMillis);
                
                if(saveGame())
                {
                    game_printMsg("Game saved successfully!",3);
                    changeState(ST_MENUSTATE);
                }
                else
                {
                    game_printMsg("Couldn't save the game!",3);
                }
                break;
            case SDL_CONTROLLER_BUTTON_B:
            case SDL_CONTROLLER_BUTTON_START:
                gamePaused = false;
                break;
            case SDL_CONTROLLER_BUTTON_X:
                changeState(ST_MENUSTATE);
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                changeState(ST_GAMESTATE);
                break;
        }
        return;
    }

    switch(button)
    {
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            doSelectorMovement(0,1);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            doSelectorMovement(0,-1);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            doSelectorMovement(-1,0);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            doSelectorMovement(1,0);
            break;
        case SDL_CONTROLLER_BUTTON_A:
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_X:
        case SDL_CONTROLLER_BUTTON_Y:
            gamelogic_clickedTile(selectedTile.x, selectedTile.y, false);
            break;
        case SDL_CONTROLLER_BUTTON_START:
            gamePausing = !gamePausing;
            if(!gamePausing)
                game_printMsg("",0);
            break;
        default:
            break;
    }
}

void gamestate_control_released(SDL_GameControllerButton button, const SDL_Event *event)
{
    if(launchedTutorial && !tutorialFinished)
        return;

    switch(button)
    {
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            moveDir.movey--;
            checkSelectorMovement();
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            moveDir.movey++;
            checkSelectorMovement();
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            moveDir.movex++;
            checkSelectorMovement();
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            moveDir.movex--;
            checkSelectorMovement();
            break;
        default:
            break;
    }
}

void gamestate_stop(void)
{
    gamedraw_destroyAssets();
    gamelogic_stop();
    ktimer_destroy(moveDir.moveTimer);
    moveDir.moveTimer = NULL;
    ktimer_destroy(gameTimer);
    gameTimer = NULL;
}
