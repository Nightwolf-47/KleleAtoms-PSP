#include "gamedraw.h"
#include "../../game/game.h"
#include "../../game/state.h"
#include "../../utils/rendertext.h"
#include "gamelogic.h"
#include "gameai.h"
#include "../../game/assetman.h"
#include <math.h>

extern SDL_Renderer* gameRenderer;

// Tile size in pixels
#define TILESIZE 31
// Atom size in pixels
#define ATOMSIZE 11

static SDL_Texture* texAtom;
static SDL_Texture* texExplode;
static SDL_Texture* texPlayer;
static SDL_Texture* texPlayerAI;
static SDL_Texture* texSelector;
static SDL_Texture* gameGrid;
static SDL_Texture* pauseButtons;

// Grid left side X position
static int gridStartX;
// Grid top side Y position
static int gridStartY;

// If the game has ended, stores the victory time, otherwise -1
static Sint32 victoryTime = -1;

static const SDL_Color atomPlayerColors[4] = {
    {255,51,51,SDL_ALPHA_OPAQUE},   //Red
    {51,102,255,SDL_ALPHA_OPAQUE},  //Blue
    {0,255,0,SDL_ALPHA_OPAQUE},     //Green
    {240,240,0,SDL_ALPHA_OPAQUE}    //Yellow
};

static const char* playerNames[4] = {"Red","Blue","Green","Yellow"};

// Generates the grid texture
static SDL_Texture* initGrid(int gridWidth, int gridHeight)
{
    SDL_Surface* imgTile = assetman_loadSurface("game/tile.png");
    if(!imgTile)
        return NULL;
    
    SDL_Surface* grid = SDL_CreateRGBSurfaceWithFormat(0, gridWidth*TILESIZE, gridHeight*TILESIZE, 1, imgTile->format->format);

    const SDL_Rect tileRect = {0,0,TILESIZE,TILESIZE};
    for(int x=0; x<gridWidth; x++)
    {
        for(int y=0; y<gridHeight; y++)
        {
            SDL_Rect gridTileRect = {x*TILESIZE,y*TILESIZE,TILESIZE,TILESIZE};
            SDL_BlitSurface(imgTile, &tileRect, grid, &gridTileRect);
        }
    }

    SDL_Texture* gridTex = SDL_CreateTextureFromSurface(gameRenderer, grid);
    SDL_FreeSurface(grid);
    SDL_FreeSurface(imgTile);
    return gridTex;
}

void gamedraw_initAssets(int gridWidth, int gridHeight)
{
    gridStartX = (SCREEN_WIDTH-gridWidth*TILESIZE) / 2;
    gridStartY = (SCREEN_HEIGHT-24-gridHeight*TILESIZE) / 2 + 20;
    victoryTime = -1;

    texAtom = assetman_loadTexture(gameRenderer, "game/atom.png");
    texExplode = assetman_loadTexture(gameRenderer, "game/explode.png");
    texPlayer = assetman_loadTexture(gameRenderer, "game/player.png");
    texPlayerAI = assetman_loadTexture(gameRenderer, "game/playerai.png");
    texSelector = assetman_loadTexture(gameRenderer, "game/selector.png");
    pauseButtons = assetman_loadTexture(gameRenderer, "game/pausebuttons.png");
    gameGrid = initGrid(gridWidth, gridHeight);

    if(!texAtom || !texExplode || !gameGrid || !texPlayer || !texPlayerAI || !texSelector || !pauseButtons)
    {
        game_errorMsg("Game: Couldn't allocate assets: (%s)",SDL_GetError());
        return;
    }
}

void gamedraw_drawGrid(void)
{
    SDL_Rect gridRect = {gridStartX, gridStartY, logicData->gridWidth*TILESIZE, logicData->gridHeight*TILESIZE};
    SDL_RenderCopy(gameRenderer, gameGrid, NULL, &gridRect);
}

void gamedraw_drawAtoms(void)
{
    for(int x=0; x<logicData->gridWidth; x++)
    {
        for(int y=0; y<logicData->gridHeight; y++)
        {
            struct KATile* curTile = &logicData->tiles[x][y];
            int basex = gridStartX+(x*TILESIZE);
            int basey = gridStartY+(y*TILESIZE);
            if(curTile->explodeTime > 0)
            {
                SDL_Rect explodeRect = {10+basex,10+basey,11,11};
                SDL_RenderCopy(gameRenderer, texExplode, NULL, &explodeRect);
            }
            else if(curTile->playerNum >= 0)
            {
                const SDL_Color* atomColor = &atomPlayerColors[curTile->playerNum];
                SDL_SetTextureColorMod(texAtom, atomColor->r, atomColor->g, atomColor->b);
                int visibleAtomCount = SDL_min(curTile->atomCount,MAX_VISIBLE_ATOMS);
                for(int i=0; i<visibleAtomCount; i++)
                {
                    struct KAAtom* curAtom = &curTile->atoms[i];
                    SDL_Rect textureRect = {curAtom->curx+basex, curAtom->cury+basey, ATOMSIZE, ATOMSIZE};
                    SDL_RenderCopy(gameRenderer, texAtom, NULL, &textureRect);
                }
            }
        }
    }
}

void gamedraw_drawHUD(Sint32 gameTime)
{
    //Draw player icons
    for(Uint32 i=0; i<4; i++)
    {
        if(logicData->playerStatus[i] == PST_NOTPRESENT)
            continue;

        SDL_Color playerColor;
        if(logicData->playerStatus[i] == PST_LOST)
            playerColor = (SDL_Color){127,127,127,SDL_ALPHA_OPAQUE};
        else
            playerColor = atomPlayerColors[i];
        int x;
        if((i % 2) == 0)
            x = (gridStartX-21)/2;
        else
            x = SCREEN_WIDTH-((gridStartX+21)/2);
        int y = (i < 2) ? (110-55) : 182;
        if(logicData->curPlayer == i)
        {
            SDL_SetRenderDrawColor(gameRenderer,255,255,255,SDL_ALPHA_OPAQUE);
            SDL_Rect highlightRect = {x-1,y-1,23,57};
            SDL_RenderFillRect(gameRenderer,&highlightRect);
        }
        SDL_SetTextureColorMod(texPlayer,playerColor.r,playerColor.g,playerColor.b);
        SDL_Rect textureRect = {x,y,21,55};
        SDL_Rect aiSourceRect = {21*aiDifficulty[i],0,21,55};
        SDL_RenderCopy(gameRenderer,texPlayer,NULL,&textureRect);
        SDL_RenderCopy(gameRenderer,texPlayerAI,&aiSourceRect,&textureRect);
    }
    //Draw timer
    if(logicData->playerWon == NOPLAYER)
    {
        char timestr[16];
        snprintf(timestr,sizeof(timestr),"%02d:%02d",gameTime / 60, gameTime % 60);
        rendertext_setTextAlignment(TEXT_ALIGN_RIGHT,SCREEN_WIDTH-4);
        rendertext_drawText(timestr,0,2);
        rendertext_setTextAlignment(TEXT_ALIGN_LEFT,0);
    }
}

void gamedraw_drawSelector(int x, int y)
{
    if(aiPlayer[logicData->curPlayer] || logicData->atomStackPos > 0 || logicData->animPlaying || logicData->curPlayer < 0)
        return;

    SDL_Color playerColor = atomPlayerColors[logicData->curPlayer];
    SDL_SetTextureColorMod(texSelector,playerColor.r,playerColor.g,playerColor.b);
    SDL_Rect textureRect = {gridStartX-4+(x*TILESIZE),gridStartY-4+(y*TILESIZE),38,38};
    SDL_RenderCopy(gameRenderer,texSelector,NULL,&textureRect);
}

// Draws an in-game window background (victory or pause window)
static void drawWindow(int winWidth, int winHeight, int winX, int winY)
{
    //Draw half-transparent rectangle to darken background
    SDL_Rect rect = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
    SDL_SetRenderDrawColor(gameRenderer,0,0,0,127);
    SDL_RenderFillRect(gameRenderer, &rect);
    //Draw victory window background
    SDL_SetRenderDrawColor(gameRenderer,127,127,127,SDL_ALPHA_OPAQUE);
    rect = (SDL_Rect){winX,winY,winWidth,winHeight};
    SDL_RenderFillRect(gameRenderer,&rect);
    SDL_SetRenderDrawColor(gameRenderer,0,0,0,SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(gameRenderer,&rect);
}

void gamedraw_drawPauseWindow(void)
{
    const int winWidth = 240;
    const int winHeight = 150;
    const int winX = (SCREEN_WIDTH-winWidth)/2;
    const int winY = (SCREEN_HEIGHT-winHeight)/2;
    const int textY = winY + 4;

    SDL_Color btnTextColor = {0,0,0,SDL_ALPHA_OPAQUE};
    SDL_Rect buttonRect = {winX+((winWidth-76)/2),winY+((winHeight-76+18)/2),76,76};
    int textMidY = buttonRect.y+(buttonRect.h-14)/2;
    drawWindow(winWidth,winHeight,winX,winY);
    rendertext_setTextAlignment(TEXT_ALIGN_CENTER,winWidth);
    rendertext_drawText("PAUSE",winX,textY);
    rendertext_drawTextColored("Restart",winX,buttonRect.y-18,btnTextColor);
    rendertext_drawTextColored("Save & Quit",winX,buttonRect.y+76+4,btnTextColor);
    rendertext_setTextAlignment(TEXT_ALIGN_RIGHT,buttonRect.x-6);
    rendertext_drawTextColored("Menu",0,textMidY,btnTextColor);
    rendertext_setTextAlignment(TEXT_ALIGN_LEFT,0);
    rendertext_drawTextColored("Resume",buttonRect.x+buttonRect.w+6,textMidY,btnTextColor);
    SDL_RenderCopy(gameRenderer, pauseButtons, NULL, &buttonRect);
}

void gamedraw_drawVictoryWindow(Sint32 gameTime)
{
    if(victoryTime < 0)
        victoryTime = gameTime;
    
    const int winWidth = 220;
    const int winHeight = 110;
    const int winX = (SCREEN_WIDTH-winWidth)/2;
    const int winY = (SCREEN_HEIGHT-winHeight)/2;
    const int textY = winY + 10;
    
    drawWindow(winWidth,winHeight,winX,winY);
    //Draw victory window text
    char text[100];
    snprintf(text,sizeof(text),"Victory!\n\n%s has won!\nTime: %02d:%02d\nPress any button to continue...",playerNames[logicData->playerWon],victoryTime / 60,victoryTime % 60);
    rendertext_setTextAlignment(TEXT_ALIGN_CENTER,winWidth);
    rendertext_drawText(text,winX,textY);
    rendertext_setTextAlignment(TEXT_ALIGN_LEFT,0);
}

void gamedraw_destroyAssets(void)
{
    if(texAtom)
    {
        SDL_DestroyTexture(texAtom);
        texAtom = NULL;
    }
    if(texExplode)
    {
        SDL_DestroyTexture(texExplode);
        texExplode = NULL;
    }
    if(texPlayer)
    {
        SDL_DestroyTexture(texPlayer);
        texPlayer = NULL;
    }
    if(texSelector)
    {
        SDL_DestroyTexture(texSelector);
        texSelector = NULL;
    }
    if(texPlayerAI)
    {
        SDL_DestroyTexture(texPlayerAI);
        texPlayerAI = NULL;
    }
    if(gameGrid)
    {
        SDL_DestroyTexture(gameGrid);
        gameGrid = NULL;
    }
    if(pauseButtons)
    {
        SDL_DestroyTexture(pauseButtons);
        pauseButtons = NULL;
    }
}
