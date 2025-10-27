#include "menuui.h"
#include "../../game/state.h"
#include "../../game/game.h"
#include "../../game/save.h"
#include "../../utils/timer.h"
#include "../../utils/rendertext.h"
#include "../../utils/pspwav.h"
#include "../../game/assetman.h"

// The full button count
#define BUTTON_COUNT 8
// How many rows of buttons are there
#define BUTTON_ROWS 2

static const int startHoldTime = -200; //Time to set the button hold timer to when button holding is started
static const int holdDelay = 150; 

// Type of menu draw element
enum MenuDEType {
    MDE_NULL,
    MDE_TEXT,
    MDE_IMAGE
};

typedef struct MenuUIButton MenuUIButton;

static const SDL_Color defaultColor = {255,255,255,255};
static const SDL_Color blackColor = {0,0,0,255};
static const SDL_Rect normalButtonRect = {0,0,90,45};
static const SDL_Rect smallButtonRect = {0,0,50,45};

// Player type names in readable format
static const char* playerTypeStrings[5] = {
    "None",
    "Human",
    "AI 1",
    "AI 2",
    "AI 3",
};

// Atom colors for each player, yet again
static const SDL_Color playerColors[4] = {
    {255,51,51,SDL_ALPHA_OPAQUE},   //Red
    {51,102,255,SDL_ALPHA_OPAQUE},  //Blue
    {0,255,0,SDL_ALPHA_OPAQUE},     //Green
    {240,240,0,SDL_ALPHA_OPAQUE}    //Yellow
};

typedef struct MenuDrawElement {
    enum MenuDEType type;
    int x;
    SDL_Color color;
    char text[8];
    SDL_Point sourceTexPos;
    SDL_Texture* img;
} MenuDrawElement;

static const SDL_Color backgroundColor = {0x80,0x80,0x80,SDL_ALPHA_OPAQUE};
static const SDL_Color outlineColor = {0xFF,0xFF,0xFF,SDL_ALPHA_OPAQUE};
static const SDL_Color outlineColorSelect = {0xFF,0xBF,0x00,SDL_ALPHA_OPAQUE};
static const SDL_Color backgroundColorSelect = {0x70,0x70,0x70,SDL_ALPHA_OPAQUE};
static const SDL_Color backgroundColorPress = {0x58,0x58,0x58,SDL_ALPHA_OPAQUE};

struct MenuUIButton {
    MenuDrawElement drawElements[2];
    void (*clickCallback)(MenuUIButton* menuButton, SDL_GameControllerButton controllerButton); //Callback called both on init (with controllerButton < 0) and when the button is clicked (controllerButton >= 0)
    SDL_Rect rect;
    const char* description;
    char descriptionAddition[16];
    int drawElementCount;
    int buttonVal;
};

struct MenuButtonHold {
    bool held;
    KTimer* holdTimer;
    SDL_GameControllerButton pressedButton;
};

static struct MenuButtonHold buttonHeld; //Struct containing data about held button (time, isHeld, which physical button is pressed)
static int selectedButton; //Selected button index

static SDL_Texture* gridWidthTex;
static SDL_Texture* gridHeightTex;
static SDL_Texture* playerTypeTex;
static SDL_Texture* aiTypeTex;
static SDL_Texture* playGameTex;
static SDL_Texture* tutorialTex;

// Returns a pointer to the player type setting of a given player
static int* getPlayerType(int playerNum)
{
    int* playerType;
    switch(playerNum)
    {
        case 0:
            playerType = &gameSettings.player1Type;
            break;
        case 1:
            playerType = &gameSettings.player2Type;
            break;
        case 2:
            playerType = &gameSettings.player3Type;
            break;
        case 3:
            playerType = &gameSettings.player4Type;
            break;
        default:
            playerType = NULL; //Invalid
            break;
    }
    if(!playerType || *playerType < 0 || *playerType > 4)
        return NULL;
    return playerType;
}

// Returns true if there are more than 2 valid players in the settings
static bool moreThan2Players(void)
{
    int playerCount = (gameSettings.player1Type > 0) + (gameSettings.player2Type > 0) + (gameSettings.player3Type > 0) + (gameSettings.player4Type > 0);
    return playerCount > 2;
}

static void tutorialClickCallback(MenuUIButton* menuButton, SDL_GameControllerButton controllerButton)
{
    if(controllerButton >= 0)
    {
        launchedTutorial = true;
        changeState(ST_GAMESTATE);
    }
    if(!menuButton->drawElements[0].img)
        menuButton->drawElements[0].img = tutorialTex;
}

static void startButtonClickCallback(MenuUIButton* menuButton, SDL_GameControllerButton controllerButton)
{
    if(controllerButton >= 0)
    {
        launchedTutorial = false;
        changeState(ST_GAMESTATE);
    }
    if(!menuButton->drawElements[0].img)
        menuButton->drawElements[0].img = playGameTex;
}

static void playerClickCallback(MenuUIButton* menuButton, SDL_GameControllerButton controllerButton)
{
    int* playerType = getPlayerType(menuButton->buttonVal);
    if(!playerType)
    {
        menuButton->drawElements[0].img = NULL;
        menuButton->drawElements[1].img = NULL;
        snprintf(menuButton->descriptionAddition,sizeof(menuButton->descriptionAddition)," (Invalid)");
        return;
    }

    if(controllerButton >= 0)
    {
        int move = 1-(2*(controllerButton == SDL_CONTROLLER_BUTTON_B || controllerButton == SDL_CONTROLLER_BUTTON_Y));
        int min = !moreThan2Players();
        int max = 4;
        *playerType += move;
        if(*playerType < min)
            *playerType = max;
        if(*playerType > max)
            *playerType = min;
    }
    if(*playerType == 0)
        menuButton->drawElements[0].color = (SDL_Color){127,127,127,SDL_ALPHA_OPAQUE};
    else
        menuButton->drawElements[0].color = playerColors[menuButton->buttonVal];
    menuButton->drawElements[1].sourceTexPos.x = (SDL_max(*playerType,1)-1) * menuButton->rect.w;
    snprintf(menuButton->descriptionAddition,sizeof(menuButton->descriptionAddition)," (%s)", playerTypeStrings[*playerType]);
    if(!menuButton->drawElements[0].img)
        menuButton->drawElements[0].img = playerTypeTex;
    if(!menuButton->drawElements[1].img)
        menuButton->drawElements[1].img = aiTypeTex;
}

static void gridSizeCallback(MenuUIButton* menuButton, SDL_GameControllerButton controllerButton)
{
    int move = 1-(2*(controllerButton == SDL_CONTROLLER_BUTTON_B || controllerButton == SDL_CONTROLLER_BUTTON_Y));
    if(menuButton->buttonVal)
    {
        if(controllerButton >= 0)
        {
            gameSettings.gridHeight += move;
            if(gameSettings.gridHeight > MAX_GRIDHEIGHT)
                gameSettings.gridHeight = MIN_GRIDHEIGHT;
            if(gameSettings.gridHeight < MIN_GRIDHEIGHT)
                gameSettings.gridHeight = MAX_GRIDHEIGHT;
        }
        if(!menuButton->drawElements[0].img)
            menuButton->drawElements[0].img = gridWidthTex;
        snprintf(menuButton->drawElements[1].text,sizeof(menuButton->drawElements[1].text),"%d",gameSettings.gridHeight);
    }
    else
    {
        if(controllerButton >= 0)
        {
            gameSettings.gridWidth += move;
            if(gameSettings.gridWidth > MAX_GRIDWIDTH)
                gameSettings.gridWidth = MIN_GRIDWIDTH;
            if(gameSettings.gridWidth < MIN_GRIDWIDTH)
                gameSettings.gridWidth = MAX_GRIDWIDTH;
        }
        if(!menuButton->drawElements[0].img)
            menuButton->drawElements[0].img = gridHeightTex;
        gameSettings.gridWidth = SDL_clamp(gameSettings.gridWidth, 5, 13);
        snprintf(menuButton->drawElements[1].text,sizeof(menuButton->drawElements[1].text),"%d",gameSettings.gridWidth);
    }
}

//List of all buttons in the game
static MenuUIButton buttons[BUTTON_COUNT] = {
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL}},
        .drawElementCount = 1,
        .clickCallback = &tutorialClickCallback,
        .rect = normalButtonRect,
        .description = "Start the tutorial."
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL}},
        .drawElementCount = 1,
        .clickCallback = &startButtonClickCallback,
        .rect = normalButtonRect,
        .description = "Start the game."
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL},{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL}},
        .drawElementCount = 2,
        .clickCallback = &playerClickCallback,
        .rect = smallButtonRect,
        .buttonVal = 0,
        .description = "Player 1 type"
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL},{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL}},
        .drawElementCount = 2,
        .clickCallback = &playerClickCallback,
        .rect = smallButtonRect,
        .buttonVal = 1,
        .description = "Player 2 type"
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL},{MDE_TEXT,60,blackColor,{0},{0,0},NULL}},
        .drawElementCount = 2,
        .clickCallback = &gridSizeCallback,
        .rect = normalButtonRect,
        .buttonVal = 0,
        .description = "Set grid width. ("__XSTRING(MIN_GRIDWIDTH)"-"__XSTRING(MAX_GRIDWIDTH)")"
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL},{MDE_TEXT,60,blackColor,{0},{0,0},NULL}},
        .drawElementCount = 2,
        .clickCallback = &gridSizeCallback,
        .rect = normalButtonRect,
        .buttonVal = 1,
        .description = "Set grid height. ("__XSTRING(MIN_GRIDHEIGHT)"-"__XSTRING(MAX_GRIDHEIGHT)")"
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL},{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL}},
        .drawElementCount = 2,
        .clickCallback = &playerClickCallback,
        .rect = smallButtonRect,
        .buttonVal = 2,
        .description = "Player 3 type"
    },
    {
        .drawElements = {{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL},{MDE_IMAGE,0,defaultColor,{0},{0,0},NULL}},
        .drawElementCount = 2,
        .clickCallback = &playerClickCallback,
        .rect = smallButtonRect,
        .buttonVal = 3,
        .description = "Player 4 type"
    },
};

// Gets the first row width in pixels
static int getRowWidth(int elemsPerRow)
{
    int width = 0;
    for(int i=0; i<elemsPerRow; i++)
    {
        width += (buttons[i].rect.w + 15);
    }
    return width - 15;
}

bool menuui_init(SDL_Renderer* renderer)
{
    gridWidthTex = assetman_loadTexture(renderer, "menu/gridwidth.png");
    gridHeightTex = assetman_loadTexture(renderer, "menu/gridheight.png");
    playerTypeTex = assetman_loadTexture(renderer, "menu/playertype1.png");
    aiTypeTex = assetman_loadTexture(renderer, "menu/playertype2.png");
    playGameTex = assetman_loadTexture(renderer, "menu/startgame.png");
    tutorialTex = assetman_loadTexture(renderer, "menu/tutorial.png");
    selectedButton = 1;
    if(isSavePresent())
    {
        buttons[1].description = "Load a saved game.";
        buttons[1].drawElements[0].sourceTexPos.y = 45;
    }  
    else
    {
        buttons[1].description = "Start the game.";
        buttons[1].drawElements[0].sourceTexPos.y = 0;
    }
    buttonHeld.holdTimer = ktimer_create();
    buttonHeld.held = false;
    int elemsPerRow = (BUTTON_COUNT/BUTTON_ROWS);
    int xoffs = (SCREEN_WIDTH - getRowWidth(elemsPerRow)) / 2;
    int yoffs = (272-45)/2 - 20;
    int curY = yoffs;
    for(int y=0; y<BUTTON_ROWS; y++)
    {
        int curX = xoffs;
        for(int x=0; x<elemsPerRow; x++)
        {
            MenuUIButton* btn = &buttons[x+(y*elemsPerRow)];
            btn->rect = (SDL_Rect){curX,curY,btn->rect.w,btn->rect.h};
            if(btn->clickCallback)
                btn->clickCallback(btn, -1);
            curX += btn->rect.w + 15;
        }
        curY += buttons[y*elemsPerRow].rect.h + 20;
    }
    return true;
}

void menuui_update(void)
{
    if(!buttonHeld.held)
        return;

    Sint64 heldTime = ktimer_getTimeMillis(buttonHeld.holdTimer);
    if(heldTime >= holdDelay)
    {
        ktimer_setTimeMillis(buttonHeld.holdTimer, 0);
        pspwav_play(sfxClick);
        if(buttons[selectedButton].clickCallback)
            buttons[selectedButton].clickCallback(&buttons[selectedButton], buttonHeld.pressedButton);
    }
}

void menuui_draw(SDL_Renderer* renderer)
{
    int oldTextWidth;
    enum TextAlignment oldAlign = rendertext_getTextAlignment(&oldTextWidth);
    for(int i=0; i<BUTTON_COUNT; i++)
    {
        SDL_Color bgCol = backgroundColor;
        SDL_Color outCol = outlineColor;
        if(i == selectedButton)
        {
            outCol = outlineColorSelect;
            if(buttonHeld.held)
                bgCol = backgroundColorPress;
            else
                bgCol = backgroundColorSelect;
        }

        SDL_SetRenderDrawColor(renderer, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
        SDL_RenderFillRect(renderer, &buttons[i].rect);
        SDL_SetRenderDrawColor(renderer, outCol.r, outCol.g, outCol.b, outCol.a);
        SDL_RenderDrawRect(renderer, &buttons[i].rect);
        if(i == selectedButton)
        {
            char desc[100];
            rendertext_setTextAlignment(TEXT_ALIGN_CENTER,SCREEN_WIDTH);
            snprintf(desc,sizeof(desc),"%s%s",buttons[i].description,buttons[i].descriptionAddition);
            rendertext_drawTextColored(desc,0,220,(SDL_Color){255,165,0,SDL_ALPHA_OPAQUE});
        }
        SDL_Rect sourceRect = {0,0,buttons[i].rect.w,buttons[i].rect.h};
        SDL_Rect destRect = {buttons[i].rect.x,buttons[i].rect.y,buttons[i].rect.w,buttons[i].rect.h};
        int textYpos = buttons[i].rect.y+((buttons[i].rect.h-14)/2);
        for(int e=0; e<buttons[i].drawElementCount; e++)
        {
            struct MenuDrawElement* curElem = &buttons[i].drawElements[e];
            switch(curElem->type)
            {
                case MDE_IMAGE:
                    if(!curElem->img)
                        break;
                    sourceRect.x = curElem->sourceTexPos.x;
                    sourceRect.y = curElem->sourceTexPos.y;
                    SDL_SetTextureAlphaMod(curElem->img,curElem->color.a);
                    SDL_SetTextureColorMod(curElem->img,curElem->color.r,curElem->color.g,curElem->color.b);
                    SDL_RenderCopy(renderer,curElem->img,&sourceRect,&destRect);
                    SDL_SetTextureAlphaMod(curElem->img,SDL_ALPHA_OPAQUE);
                    SDL_SetTextureColorMod(curElem->img,255,255,255);
                    break;
                case MDE_TEXT:
                    rendertext_setTextAlignment(TEXT_ALIGN_LEFT,0);
                    rendertext_drawTextColored(curElem->text,curElem->x+buttons[i].rect.x,textYpos,curElem->color);
                    break;
            }
        }
    }
    rendertext_setTextAlignment(oldAlign,oldTextWidth);
    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
}

// Moves the button selection
static void moveButton(int movex, int movey)
{
    if(movex == 0 && movey == 0)
        return;
    
    buttonHeld.held = false;
    buttonHeld.pressedButton = -1;
    int rowButtonCount = (BUTTON_COUNT/BUTTON_ROWS);
    int newx = (selectedButton % rowButtonCount);
    int newy = (selectedButton / rowButtonCount);
    newx = (newx + movex + rowButtonCount) % rowButtonCount;
    newy = (newy + movey + BUTTON_ROWS) % BUTTON_ROWS;
    selectedButton = newy*rowButtonCount+newx;
    if(buttons[selectedButton].clickCallback)
        buttons[selectedButton].clickCallback(&buttons[selectedButton], -1);
}

// Moves the selection based on a D-Pad direction
static bool moveDpad(SDL_GameControllerButton pressedButton)
{
    switch(pressedButton)
    {
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            moveButton(0,1);
            return true;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            moveButton(0,-1);
            return true;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            moveButton(-1,0);
            return true;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            moveButton(1,0);
            return true;
        default:
            return false;
    }
}

// Checks if the pressed button is valid in the context of menu UI
static bool isButtonValid(SDL_GameControllerButton pressedButton)
{
    switch(pressedButton)
    {
        case SDL_CONTROLLER_BUTTON_A:
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_X:
        case SDL_CONTROLLER_BUTTON_Y:
        case SDL_CONTROLLER_BUTTON_START:
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return true;
        default:
            return false;
    }
}

void menuui_press(SDL_GameControllerButton pressedButton)
{
    if(!isButtonValid(pressedButton) || moveDpad(pressedButton))
        return;

    if(!buttonHeld.held || buttonHeld.pressedButton != pressedButton)
    {   
        buttonHeld.held = true;
        ktimer_setTimeMillis(buttonHeld.holdTimer, startHoldTime);
        buttonHeld.pressedButton = pressedButton;
        pspwav_play(sfxClick);
        if(buttons[selectedButton].clickCallback)
            buttons[selectedButton].clickCallback(&buttons[selectedButton], buttonHeld.pressedButton);
    }
}

void menuui_release(SDL_GameControllerButton releasedButton)
{
    if(!isButtonValid)
        return;

    if(buttonHeld.held && buttonHeld.pressedButton == releasedButton)
    {
        buttonHeld.held = false;
        buttonHeld.pressedButton = -1;
    }
}

// Removes the pointers for all the button images, done during menu UI cleanup in menuui_stop
static void removeAllImages(void)
{
    for(int i=0; i<BUTTON_COUNT; i++)
    {
        for(int e=0; e<buttons[i].drawElementCount; e++)
        {
            buttons[i].drawElements[e].img = NULL;
        }
    }
}

void menuui_stop(void)
{
    removeAllImages();
    if(gridWidthTex)
    {
        SDL_DestroyTexture(gridWidthTex);
        gridWidthTex = NULL;
    }
    if(gridHeightTex)
    {
        SDL_DestroyTexture(gridHeightTex);
        gridHeightTex = NULL;
    }
    if(playerTypeTex)
    {
        SDL_DestroyTexture(playerTypeTex);
        playerTypeTex = NULL;
    }
    if(aiTypeTex)
    {
        SDL_DestroyTexture(aiTypeTex);
        aiTypeTex = NULL;
    }
    if(playGameTex)
    {
        SDL_DestroyTexture(playGameTex);
        playGameTex = NULL;
    }
    if(tutorialTex)
    {
        SDL_DestroyTexture(tutorialTex);
        tutorialTex = NULL;
    }

    ktimer_destroy(buttonHeld.holdTimer);
    buttonHeld.holdTimer = NULL;
    buttonHeld.held = false;
}
