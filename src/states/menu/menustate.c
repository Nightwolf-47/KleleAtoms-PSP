#include "menustate.h"
#include "menuatoms.h"
#include "menuui.h"
#include "../../game/game.h"
#include "../../game/state.h"
#include "../../utils/rendertext.h"
#include "../../game/assetman.h"
#include <SDL2/SDL_render.h>

static SDL_Texture* bgImage;
static SDL_Texture* logoImage;
static SDL_Texture* menuAtomImage;

//Game version string shown in the bottom left corner
const char* versionStr = "v1.0";

//Atom colors for each player
static const SDL_Color atomPlayerColors[4] = {
    {255,51,51,SDL_ALPHA_OPAQUE},   //Red
    {51,102,255,SDL_ALPHA_OPAQUE},  //Blue
    {0,255,0,SDL_ALPHA_OPAQUE},     //Green
    {240,240,0,SDL_ALPHA_OPAQUE}    //Yellow
};

void menustate_init(SDL_Renderer* rend)
{
    bgImage = assetman_loadTexture(rend, "menu/menubg.png");
    logoImage = assetman_loadTexture(rend, "menu/logo.png");
    menuAtomImage = assetman_loadTexture(rend, "menu/menuatoms.png");
    bool uiInit = menuui_init(rend);
    if(!bgImage || !logoImage || !menuAtomImage || !uiInit)
    {
        game_errorMsg("Menu: Couldn't allocate assets! (%s)",SDL_GetError());
        return;
    }
    menuAtomCount = 0;
}

void menustate_update(float dt)
{
    menuatoms_spawnAtom(atomPlayerColors[rand() & 3]);
    menuatoms_moveAtoms(dt);
    menuui_update();
}

void menustate_draw(SDL_Renderer* rend)
{
    SDL_Rect textureRect;
    textureRect = (SDL_Rect){0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
    SDL_RenderCopy(rend, bgImage, NULL, &textureRect);
    for(int i=0; i<menuAtomCount; i++)
    {
        SDL_SetTextureColorMod(menuAtomImage, menuAtoms[i].atomColor.r, menuAtoms[i].atomColor.g, menuAtoms[i].atomColor.b);
        SDL_Rect sourceRect = {menuAtoms[i].atomType*29,0,29,29};
        SDL_Rect targetRect = {menuAtoms[i].x, menuAtoms[i].y, 29, 29};
        SDL_RenderCopy(rend, menuAtomImage, &sourceRect, &targetRect);
    }
    textureRect = (SDL_Rect){133,12,213,70};
    SDL_RenderCopy(rend, logoImage, NULL, &textureRect);
    menuui_draw(rend);
    rendertext_setTextAlignment(TEXT_ALIGN_RIGHT,SCREEN_WIDTH-4);
    rendertext_drawText("Made by Nightwolf-47",0,SCREEN_HEIGHT-16);
    rendertext_setTextAlignment(TEXT_ALIGN_LEFT,0);
    rendertext_drawText(versionStr,4,SCREEN_HEIGHT-16);
}

void menustate_control_pressed(SDL_GameControllerButton button, const SDL_Event *event)
{
    menuui_press(button);
}

void menustate_control_released(SDL_GameControllerButton button, const SDL_Event *event)
{
    menuui_release(button);
}

void menustate_stop(void)
{
    menuui_stop();
    if(bgImage)
    {
        SDL_DestroyTexture(bgImage);
        bgImage = NULL;
    }
    if(logoImage)
    {
        SDL_DestroyTexture(logoImage);
        logoImage = NULL;
    }
    if(menuAtomImage)
    {
        SDL_DestroyTexture(menuAtomImage);
        menuAtomImage = NULL;
    }
}
