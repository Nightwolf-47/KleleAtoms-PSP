#include "gamelogic.h"
#include "../../game/state.h"
#include "../../game/game.h"
#include "../../utils/wavplayer.h"
#include "gameai.h"
#include <SDL2/SDL.h>
#include <limits.h>
#include <stdlib.h>

// Base atom speed in pixels per second
const static float baseAtomSpeed = 42.27f;

// X and Y Position of atoms in the middle of a tile
const static int atomMidPos = 10;

// Position of atoms on the right (X) and/or down (Y) side of the tile
const static int atomEndPos = 18;

struct GameLogicData* logicData = NULL;

// Order of nearby critical tile checks during chain reactions (stored as position offsets from checked tile)
const static Vec2 checkTab[4] = {
    {0,1},{0,-1},{1,0},{-1,0}
};

// Switch the current player to the next one and reset AI timer
static void nextPlayer(void)
{   
    ai_ResetTime();
    if(logicData->curPlayerCount < 2)
        return;
    
    do
    {
        logicData->curPlayer++;
        logicData->curPlayer &= 3;
    }
    while(logicData->playerStatus[logicData->curPlayer] <= PST_LOST);
}

/// @brief Put an atom (or multiple atoms) on a given tile and play atom animations
/// @param x Tile X position
/// @param y Tile Y position
/// @param newPlayer Set the player the atoms on that tile will belong to. If newPlayer == NOPLAYER, newPlayer is current player.
/// @param count How many atoms will be placed
/// @return true if the tile becomes (or is) critical
static bool putAtom(int x, int y, int newPlayer, int count)
{
    if(x < 0 || x >= logicData->gridWidth || y < 0 || y >= logicData->gridHeight)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"putAtom: invalid position (%d, %d)",x,y);
        return false;
    }
    struct KATile* curTile = &logicData->tiles[x][y];

    int oldPlayer = curTile->playerNum;
    if(newPlayer == NOPLAYER)
        curTile->playerNum = logicData->curPlayer;
    else
        curTile->playerNum = newPlayer;
    int atPlayer = curTile->playerNum;
    for(int i=0; i<count; i++)
    {
        if(curTile->atomCount >= MAX_VISIBLE_ATOMS)
        {
            curTile->atomCount++;
            continue;
        }

        switch(curTile->atomCount)
        {
            case 0:
                curTile->atoms[curTile->atomCount++] = (struct KAAtom){atomMidPos,atomMidPos,atomMidPos,atomMidPos};
                break;
            case 1:
                logicData->animPlaying = true;
                curTile->atoms[0].endx = atomEndPos;
                curTile->atoms[curTile->atomCount++] = (struct KAAtom){atomMidPos,atomMidPos,2,atomMidPos};
                break;
            case 2:
                logicData->animPlaying = true;
                curTile->atoms[0].endy = 2;
                curTile->atoms[1].endy = 2;
                curTile->atoms[curTile->atomCount++] = (struct KAAtom){atomMidPos,atomMidPos,atomMidPos,atomEndPos};
                break;
            case 3:
                logicData->animPlaying = true;
                curTile->atoms[2].endx = 2;
                curTile->atoms[curTile->atomCount++] = (struct KAAtom){atomMidPos,atomEndPos,atomEndPos,atomEndPos};
                break;
            default:
                curTile->atoms[curTile->atomCount++] = (struct KAAtom){atomMidPos,atomMidPos,(rand() % 11)+5,(rand() % 11)+5};
                break;
        }
    }
    return (atPlayer != NOPLAYER && curTile->atomCount >= logicData->critGrid[x][y]);
}

/// @brief Blow up a critical tile, which spreads the atoms to nearby tiles
/// @param x Tile X position
/// @param y Tile Y position
static void explodeAtoms(int x, int y)
{
    logicData->explosionCount++;
    struct KATile* curTile = &logicData->tiles[x][y];
    float explosionTimeMultiplier = SDL_min(logicData->explosionCount,1000)/10.0f;
    curTile->explodeTime = 0.3f/SDL_max(explosionTimeMultiplier,1);
    int atplayer = curTile->playerNum;
    int extra = SDL_max(curTile->atomCount-logicData->critGrid[x][y],0);
    if(y < logicData->gridHeight-1)
        { putAtom(x,y+1,atplayer,extra+1); extra = 0; }
    if(y > 0)
        { putAtom(x,y-1,atplayer,extra+1); extra = 0; }
    if(x < logicData->gridWidth-1)
        { putAtom(x+1,y,atplayer,extra+1); extra = 0; }
    if(x > 0)
        { putAtom(x-1,y,atplayer,extra+1); extra = 0; }
    curTile->playerNum = NOPLAYER;
    curTile->atomCount = 0;
    if(logicData->explosionCount < 1000)
        wavplayer_play(sfxExplode);
}

/// @brief Checks if any surrounding tiles are critical
/// @param x Base tile X position
/// @param y Base tile Y position
/// @return Position of a surrounding tile that's critical or [INT_MAX, INT_MAX] vector if none of those tiles is critical
static Vec2 checkSurrounding(int x, int y)
{
    for(int i=0; i<4; i++)
    {
        int nx = x+checkTab[i].x;
        int ny = y+checkTab[i].y;
        if(nx >= 0 && nx < logicData->gridWidth && ny >= 0 && ny < logicData->gridHeight)
        {
            struct KATile* curTile = &logicData->tiles[nx][ny];
            if(curTile->playerNum != NOPLAYER && curTile->atomCount >= logicData->critGrid[nx][ny])
            {
                return (Vec2){nx,ny};
            }
        }
    }
    return (Vec2){INT_MAX,INT_MAX};
}

/// @brief Put an atom and mark the tile for explosion if critical
/// @param x Tile X position
/// @param y Tile Y position
static void prepareNewAtoms(int x, int y)
{
    Vec2 tilePos = {x,y};
    logicData->playerAtoms[logicData->curPlayer] = SDL_max(logicData->playerAtoms[logicData->curPlayer], 1);
    if(putAtom(x,y,NOPLAYER,1))
    {
        logicData->willExplode = (struct KAWillExplode){tilePos.x,tilePos.y,true};
        logicData->atomStack[logicData->atomStackPos++] = tilePos;
    }
    else
    {
        nextPlayer();
    }
}

/// @brief Check for animations, move atoms, decrease explosion timers and calculate player atom counts
/// @param dt DeltaTime from update state callback, used for atom movement speed calculation
static void doTileActions(float dt)
{
    logicData->animPlaying = false;
    memset(logicData->playerAtoms, 0, sizeof(logicData->playerAtoms));
    float explosionSpeedMultiplier = SDL_min(logicData->explosionCount,1000)/10.0f;
    float atomMoveSpeed = baseAtomSpeed*dt*SDL_max(explosionSpeedMultiplier, 1);
    for(int x=0; x<logicData->gridWidth; x++)
    {
        for(int y=0; y<logicData->gridHeight; y++)
        {
            struct KATile* curTile = &logicData->tiles[x][y];
            if(curTile->explodeTime > 0)
            {
                curTile->explodeTime -= dt;
                if(curTile->explodeTime <= 0)
                    curTile->explodeTime = 0.0f;
                else
                    logicData->animPlaying = true;
            }
            else if(curTile->playerNum >= 0)
            {
                logicData->playerAtoms[curTile->playerNum] += curTile->atomCount;
                //Move animated atoms
                for(int i=0; i<curTile->atomCount; i++)
                {
                    struct KAAtom* curAtom = &curTile->atoms[i];
                    //If the atom isn't at its target position, move it
                    if((curAtom->curx != curAtom->endx) || (curAtom->cury != curAtom->endy))
                    {
                        logicData->animPlaying = true;
                        float xmove = SDL_clamp(curAtom->endx-curAtom->curx,-atomMoveSpeed,atomMoveSpeed);
                        float ymove = SDL_clamp(curAtom->endy-curAtom->cury,-atomMoveSpeed,atomMoveSpeed);
                        curAtom->curx += xmove;
                        curAtom->cury += ymove;
                    }
                }
            }
        }
    }
}

void gamelogic_setAtoms(int x, int y, int player, uint32_t atomCount)
{
    if(x < 0 || x >= logicData->gridWidth || y < 0 || y >= logicData->gridHeight)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"setAtoms: invalid position (%d, %d)",x,y);
        return;
    }

    struct KATile* curTile = &logicData->tiles[x][y];
    curTile->explodeTime = 0;
    curTile->atomCount = atomCount;
    curTile->playerNum = player;
    switch(atomCount)
    {
        case 0:
            curTile->playerNum = NOPLAYER;
            break;
        case 1:
            curTile->atoms[0] = (struct KAAtom){atomMidPos,atomMidPos,atomMidPos,atomMidPos};
            break;
        case 2:
            curTile->atoms[0] = (struct KAAtom){2,atomMidPos,2,atomMidPos};
            curTile->atoms[1] = (struct KAAtom){atomEndPos,atomMidPos,atomEndPos,atomMidPos};
            break;
        case 3:
            curTile->atoms[0] = (struct KAAtom){2,2,2,2};
            curTile->atoms[1] = (struct KAAtom){atomEndPos,2,atomEndPos,2};
            curTile->atoms[2] = (struct KAAtom){atomMidPos,atomEndPos,atomMidPos,atomEndPos};
            break;
        default:
            curTile->atoms[0] = (struct KAAtom){2,atomEndPos,2,atomEndPos};
            curTile->atoms[1] = (struct KAAtom){atomEndPos,atomEndPos,atomEndPos,atomEndPos};
            curTile->atoms[2] = (struct KAAtom){2,2,2,2};
            curTile->atoms[3] = (struct KAAtom){atomEndPos,2,atomEndPos,2};
            break;
    }
    int visibleAtomCount = SDL_min(atomCount,MAX_VISIBLE_ATOMS);
    for(int i=4; i<visibleAtomCount; i++)
    {
        int ax = (rand() % 11)+5;
        int ay = (rand() % 11)+5;
        curTile->atoms[i] = (struct KAAtom){ax,ay,ax,ay};
    }
}

void gamelogic_setPlayers(int (*_playerTypes)[4])
{
    logicData->curPlayerCount = 0;
    int* playerTypes = *_playerTypes;
    for(int i=0; i<4; i++)
    {
        if(playerTypes[i] > 0)
        {
            if(logicData->curPlayer == NOPLAYER)
                logicData->curPlayer = i;
            
            logicData->curPlayerCount++;
            logicData->playerStatus[i] = PST_NOTSTARTED;
            logicData->playerAtoms[i] = 0;
            if(playerTypes[i] > 1)
            {
                aiPlayer[i] = true;
                aiDifficulty[i] = playerTypes[i] - 1;
            }
            else
            {
                aiPlayer[i] = false;
            }
        }
        else
        {
            logicData->playerStatus[i] = PST_NOTPRESENT;
        }
    }
    logicData->totalPlayerCount = logicData->curPlayerCount;
}

void gamelogic_init(int gridWidth, int gridHeight, int (*playerTypes)[4])
{
    if(logicData)
        gamelogic_stop();

    if(gridWidth*gridHeight > MAX_GRID_WIDTH*MAX_GRID_HEIGHT)
    {
        game_errorMsg("Game: Grid size is above maximum! (%d > %d)",gridWidth*gridHeight,MAX_GRID_WIDTH*MAX_GRID_HEIGHT);
        return;
    }
    logicData = calloc(1,sizeof(struct GameLogicData));
    logicData->gridWidth = gridWidth;
    logicData->gridHeight = gridHeight;
    for(int x=0; x<gridWidth; x++) //Grid setup
    {
        for(int y=0; y<gridHeight; y++)
        {
            struct KATile* curTile = &logicData->tiles[x][y];
            curTile->explodeTime = 0;
            curTile->atomCount = 0;
            curTile->playerNum = NOPLAYER;
            logicData->critGrid[x][y] = 4;
            if(x == 0 || x == gridWidth-1)
                logicData->critGrid[x][y]--;
            if(y == 0 || y == gridHeight-1)
                logicData->critGrid[x][y]--;
        }
    }
    logicData->animPlaying = false;
    logicData->curPlayer = NOPLAYER;
    logicData->curPlayerCount = 0;
    memset(aiPlayer, 0, sizeof(aiPlayer));
    memset(aiDifficulty, 0, sizeof(aiDifficulty));
    ai_Init();
    gamelogic_setPlayers(playerTypes);
    logicData->atomStackPos = 0;
    logicData->playerWon = NOPLAYER;
    logicData->explosionCount = 0;
    if(logicData->curPlayer == NOPLAYER || logicData->totalPlayerCount < 2)
        game_printMsg("2 or more players required to play!",3);
}

void gamelogic_tick(float dt)
{
    if(logicData->explosionCount >= ATOMSTACKSIZE)
    {
        game_printMsg("More than "XSTR(ATOMSTACKSIZE)" simultaneous explosions! Stopping...",4);
        changeState(ST_MENUSTATE);
        return;
    }
    doTileActions(dt);
    for(int i=0; i<4; i++)
    {
        if(logicData->playerStatus[i] == PST_PLAYING && logicData->playerAtoms[i] <= 0)
        {
            logicData->curPlayerCount--;
            logicData->playerStatus[i] = PST_LOST;
        }
    }
    if(logicData->curPlayerCount < 2)
    {
        for(int i=0; i<4; i++)
        {
            if(logicData->playerStatus[i] == PST_PLAYING)
            {
                logicData->playerWon = i;
                return;
            }
        }
    }
    while(logicData->playerStatus[logicData->curPlayer] <= PST_LOST)
    {
        logicData->curPlayer++;
        if(logicData->curPlayer > 3)
            logicData->curPlayer = 0;
    }
    if(!logicData->animPlaying)
    {
        if(logicData->willExplode.explode)
        {
            explodeAtoms(logicData->willExplode.x, logicData->willExplode.y);
            logicData->willExplode.explode = false;
        }
        else if(logicData->atomStackPos > 0)
        {
            int maxStackPops = SDL_min(logicData->atomStackPos,50);
            for(int i=0; i<maxStackPops; i++)
            {
                Vec2* curPos = &logicData->atomStack[logicData->atomStackPos-1];
                Vec2 pmoveTab = checkSurrounding(curPos->x, curPos->y);
                if(pmoveTab.x != INT_MAX)
                {
                    logicData->willExplode = (struct KAWillExplode){pmoveTab.x, pmoveTab.y, true};
                    logicData->atomStack[logicData->atomStackPos++] = pmoveTab;
                    break;
                }
                else
                {
                    logicData->atomStackPos--;
                    if(logicData->atomStackPos == 0)
                    {
                        nextPlayer();
                        break;
                    }
                }
            }
        }
        else if(aiPlayer[logicData->curPlayer])
        {
            ai_TryMove();
        }
    }
}

void gamelogic_clickedTile(int x, int y, bool isAIMove)
{
    int tilePlayer = logicData->tiles[x][y].playerNum;
    if(tilePlayer >= 0 && tilePlayer != logicData->curPlayer)
        return;
    if(!isAIMove && aiPlayer[logicData->curPlayer])
        return;
    if(logicData->curPlayerCount < 2 || logicData->animPlaying || logicData->atomStackPos > 0 || logicData->willExplode.explode)
        return;
    logicData->explosionCount = 0;
    logicData->playerStatus[logicData->curPlayer] = PST_PLAYING;
    wavplayer_play(sfxPut);
    prepareNewAtoms(x,y);
}

void gamelogic_stop(void)
{
    free(logicData);
    logicData = NULL;
}
