#include "gametutorial.h"
#include "gamelogic.h"
#include "../../utils/timer.h"
#include "../../utils/rendertext.h"
#include "../../game/state.h"
#include "../../game/game.h"

//Tutorial event types. Player numbers here correspond to visible player numbers (1-4), not the internal ones (0-3)
enum TutorialEventType {
    TEV_NONE, // No event
    TEV_TEXTBOX, // Draw a textbox: [text = textbox string]
    TEV_PLAYERTYPES, // Set player types: [valX (X = 1-4) = player X type (Types: 0 - None, 1 - Human, 2 - AI1, 3 - AI2, 4 - AI3)]
    TEV_CURPLAYER, // Change current player: [val1 = new current player value (1-4)]
    TEV_PUTATOM, // Place an atom as if a player did it: [val1 = player number, val2 = x, val3 = y]
    TEV_SETATOMS, // Modify tile atoms: [val1 = player number, val2 = x, val3 = y, val4 = atom count]
    TEV_WAIT, // Wait for specified time [val1 = milliseconds to wait for]
    TEV_CLEAR, // Clear the entire grid and reset player moved statuses [no parameters]
    TEV_QUIT, // Leaves the game and goes back to menu
};

typedef struct TutorialEvent {
    enum TutorialEventType type;
    int val1;
    int val2;
    int val3;
    int val4;
    const char* text;
} TutorialEvent;

// List of tutorial events
static const TutorialEvent events[] = {
    {TEV_PLAYERTYPES, 1, 1, 1, 1},
    {TEV_TEXTBOX, .text = "Welcome to the KleleAtoms tutorial! It will teach you all the game rules.\n\nYou can always press START button to quit the tutorial.\n\nPress any other button to continue..."},
    {TEV_TEXTBOX, .text = "Each player can place one atom per turn on empty or their own tiles."},
    {TEV_SETATOMS, 3, 3, 1, 1},
    {TEV_SETATOMS, 4, 4, 1, 2},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 1, 1, 1},
    {TEV_CURPLAYER, 2},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 2, 2, 1},
    {TEV_CURPLAYER, 3},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 3, 3, 1},
    {TEV_CURPLAYER, 4},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 4, 4, 1},
    {TEV_CURPLAYER, 1},
    {TEV_WAIT, 1000},
    {TEV_TEXTBOX, .text = "The atoms will explode if too many are present on one tile.\n\nAtoms in the corners explode if 2 or more are present."},
    {TEV_CLEAR},
    {TEV_SETATOMS, 1, 0, 0, 1},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 1, 0, 0},
    {TEV_WAIT, 1000},
    {TEV_TEXTBOX, .text = "On the sides they explode if 3 or more are present."},
    {TEV_CLEAR},
    {TEV_SETATOMS, 2, 0, 1, 2},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 2, 0, 1},
    {TEV_WAIT, 1000},
    {TEV_TEXTBOX, .text = "Anywhere else they only explode if 4 or more are present."},
    {TEV_CLEAR},
    {TEV_SETATOMS, 3, 1, 1, 3},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 3, 1, 1},
    {TEV_WAIT, 1000},
    {TEV_TEXTBOX, .text = "Atom explosions can make nearby atoms explode, causing chain\nreactions."},
    {TEV_CLEAR},
    {TEV_SETATOMS, 4, 1, 1, 3},
    {TEV_SETATOMS, 4, 0, 0, 1},
    {TEV_SETATOMS, 4, 1, 0, 2},
    {TEV_SETATOMS, 4, 0, 1, 2},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 4, 1, 1},
    {TEV_WAIT, 1000},
    {TEV_TEXTBOX, .text = "Exploding atoms turn nearby enemy atoms into current player's atoms."},
    {TEV_CLEAR},
    {TEV_CURPLAYER, 1},
    {TEV_SETATOMS, 1, 1, 1, 3},
    {TEV_SETATOMS, 2, 0, 1, 1},
    {TEV_SETATOMS, 3, 1, 0, 1},
    {TEV_SETATOMS, 4, 2, 1, 1},
    {TEV_SETATOMS, 2, 1, 2, 1},
    {TEV_SETATOMS, 3, 4, 4, 1},
    {TEV_WAIT, 500},
    {TEV_PUTATOM, 1, 1, 1},
    {TEV_WAIT, 1000},
    {TEV_TEXTBOX, .text = "If the player loses all their atoms, they're out.\n\nThe last standing player wins the game.\n\nThat's it!"},
    {TEV_QUIT},
};

//Number of tutorial events
const static int eventCount = (sizeof(events)/sizeof(events[0]));

static const char* currentText = NULL;  //If not NULL, a textbox with given text is being shown
static int parsedEvent = -1;            //Last parsed event number (-1 if none were parsed yet)
static KTimer* waitTimer;               //Timer used for TEV_WAIT event to wait for X milliseconds

bool tutorialFinished = true;

//Clears the entire grid and resets player statuses (TEV_CLEAR event)
static void clearGrid(void)
{
    for(int i=0; i<4; i++)
    {
        if(logicData->playerStatus[i] != PST_NOTPRESENT)
            logicData->playerStatus[i] = PST_NOTSTARTED;
    }
    for(int x=0; x<logicData->gridWidth; x++)
    {
        for(int y=0; y<logicData->gridHeight; y++)
        {
            logicData->tiles[x][y].playerNum = NOPLAYER;
            logicData->tiles[x][y].atomCount = 0;
            logicData->tiles[x][y].explodeTime = 0;
        }
    }
}

/// @brief Parses and executes the next event and sets the tutorialFinished variable to true if there no more events
/// @return true if tutorial is finished, false otherwise
static bool parseNext(void)
{
    parsedEvent++;
    if(parsedEvent >= eventCount)
    {
        tutorialFinished = true;
        return true;
    }
    const TutorialEvent* curEvent = &events[parsedEvent];
    int playerTypes[4];
    int player;
    switch (curEvent->type)
    {
        case TEV_PLAYERTYPES:
            playerTypes[0] = curEvent->val1;
            playerTypes[1] = curEvent->val2;
            playerTypes[2] = curEvent->val3;
            playerTypes[3] = curEvent->val4;
            gamelogic_setPlayers(&playerTypes);
            break;
        case TEV_CURPLAYER:
            logicData->curPlayer = curEvent->val1 - 1;
            break;
        case TEV_TEXTBOX:
            currentText = curEvent->text;
            break;
        case TEV_PUTATOM:
            logicData->curPlayer = (curEvent->val1 > 0) ? (curEvent->val1 - 1) : logicData->curPlayer;
            gamelogic_clickedTile(curEvent->val2, curEvent->val3, true);
            break;
        case TEV_SETATOMS:
            player = (curEvent->val1 > 0) ? (curEvent->val1 - 1) : logicData->curPlayer;
            gamelogic_setAtoms(curEvent->val2, curEvent->val3, player, curEvent->val4);
            break;
        case TEV_WAIT:
            ktimer_setTimeMillis(waitTimer, -curEvent->val1);
            break;
        case TEV_CLEAR:
            clearGrid();
            break;
        case TEV_QUIT:
            tutorialFinished = true;
            changeState(ST_MENUSTATE);
            break;
        case TEV_NONE:
            break;
        default:
            game_printMsg("Tutorial event has incorrect event type!", 5);
            changeState(ST_MENUSTATE);
            tutorialFinished = true;
            return false;
    }
    return false;
}

void gametutorial_init(void)
{
    waitTimer = ktimer_create();
    currentText = NULL;
    parsedEvent = -1;
    tutorialFinished = false;
}

bool gametutorial_update(void)
{
    if(tutorialFinished || !logicData)
        return false;

    while(!logicData->animPlaying && (logicData->atomStackPos == 0) && !currentText)
    {
        if(ktimer_getTimeMillis(waitTimer) < 0)
            break;
        if(parseNext())
            return false;
    }

    if(currentText)
        return true;
    else
        return false;
}

void gametutorial_draw(SDL_Renderer* renderer)
{
    if(tutorialFinished)
        return;

    if(currentText)
    {
        // Draw background dim rectangle
        SDL_Rect rect = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer,0,0,0,127);
        SDL_RenderFillRect(renderer, &rect);
        // Draw textbox background
        rect = (SDL_Rect){0,SCREEN_HEIGHT/2,SCREEN_WIDTH,SCREEN_HEIGHT/2};
        SDL_SetRenderDrawColor(renderer,127,127,127,SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &rect);
        // Draw textbox outline
        rect = (SDL_Rect){2,SCREEN_HEIGHT/2+2,SCREEN_WIDTH-4,SCREEN_HEIGHT/2-4};
        SDL_SetRenderDrawColor(renderer,0,255,0,SDL_ALPHA_OPAQUE);
        SDL_RenderDrawRect(renderer, &rect);
        // Draw text
        rendertext_drawText(currentText,8,SCREEN_HEIGHT/2+8);
    }
}

bool gametutorial_press(SDL_GameControllerButton button)
{
    if(button == SDL_CONTROLLER_BUTTON_START)
    {
        changeState(ST_MENUSTATE);
        return true;
    }

    if(tutorialFinished)
        return false;

    if(currentText)
        currentText = NULL;
    
    return true;
}

void gametutorial_stop(void)
{
    currentText = NULL;
    ktimer_destroy(waitTimer);
    waitTimer = NULL;
    tutorialFinished = true;
}
