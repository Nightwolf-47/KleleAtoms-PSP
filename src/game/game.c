#include "game.h"
#include "state.h"
#include "save.h"
#include "fade.h"
#include "assetman.h"
#include "../utils/timer.h"
#include "../utils/rendertext.h"
#include <time.h>

//PSP RTC tick functions are more accurate on that platform than SDL2 PerformanceCounter
//PSP_DISABLE_AUTOSTART_PTHREAD means pthread functions won't be linked with the program
#ifdef __PSP__
#include <psprtc.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspmoduleinfo.h>
PSP_DISABLE_AUTOSTART_PTHREAD();
#else
#include <SDL2/SDL_timer.h>
#endif

#include <SDL2/SDL_image.h>

struct KAMessage {
    const char* str;
    float time;
    bool isShown;
};

SDL_Window* gameWindow;
SDL_Renderer* gameRenderer; 

WavInfo* sfxExplode;
WavInfo* sfxPut;
WavInfo* sfxClick;

struct KASettings gameSettings;

bool launchedTutorial = false;

static struct KAMessage messageData;

static Uint64 getTimeTicks(void)
{
    Uint64 ticks;
    #ifdef __PSP__
        sceRtcGetCurrentTick(&ticks);
    #else
        ticks = SDL_GetPerformanceCounter();
    #endif
    return ticks;
}

static Uint32 getTickFrequency(void)
{
    #ifdef __PSP__
        return sceRtcGetTickResolution();
    #else
        return (Uint32)SDL_GetPerformanceFrequency();
    #endif
}

static void initSettings(void)
{
    gameSettings.gridWidth = 10;
    gameSettings.gridHeight = 6;
    gameSettings.player1Type = 1;
    gameSettings.player2Type = 3;
    gameSettings.player3Type = 0;
    gameSettings.player4Type = 0;
}

/// @brief Checks if there are at least 2 players configured in the settings
/// @return true if there are >= 2 players, false if there are < 2 players
static bool isEnoughPlayers(void)
{
    int playerCount = ((gameSettings.player1Type != 0) + (gameSettings.player2Type != 0) + (gameSettings.player3Type != 0) + (gameSettings.player4Type != 0));
    return playerCount >= 2;
}

// Clamps the settings to their proper values and resets the player types if they're invalid
static void clampSettings(void)
{
    int gridWidth = SDL_clamp(gameSettings.gridWidth, MIN_GRIDWIDTH, MAX_GRIDWIDTH);
    int gridHeight = SDL_clamp(gameSettings.gridHeight, MIN_GRIDHEIGHT, MAX_GRIDHEIGHT);
    gameSettings.player1Type = SDL_clamp(gameSettings.player1Type, 0, 4);
    gameSettings.player2Type = SDL_clamp(gameSettings.player2Type, 0, 4);
    gameSettings.player3Type = SDL_clamp(gameSettings.player3Type, 0, 4);
    gameSettings.player4Type = SDL_clamp(gameSettings.player4Type, 0, 4);
    if(!isEnoughPlayers())
        initSettings();
    gameSettings.gridWidth = gridWidth;
    gameSettings.gridHeight = gridHeight;
}

static void getCurrentDate(char* dateTime, size_t len)
{
    time_t tempTime = time(NULL);
    struct tm* curTime = localtime(&tempTime);
    strftime(dateTime, len, "%F %T", curTime);
}

static void drawMessage(float dt)
{
    if(messageData.isShown)
    {
        int oldWidth;
        enum TextAlignment oldTextAlign = rendertext_getTextAlignment(&oldWidth);
        rendertext_setTextAlignment(TEXT_ALIGN_CENTER,SCREEN_WIDTH);
        rendertext_drawTextColored(messageData.str,0,1,(SDL_Color){0,255,0,SDL_ALPHA_OPAQUE});
        rendertext_setTextAlignment(oldTextAlign,oldWidth);
        messageData.time -= dt;
        if(messageData.time <= 0)
            messageData.isShown = false;
    }
}

static bool initSounds(void)
{
    wavplayer_init();
    sfxExplode = assetman_loadWav("sfx/explode.wav");
    sfxPut = assetman_loadWav("sfx/put.wav");
    sfxClick = assetman_loadWav("sfx/click.wav");
    return (sfxExplode && sfxPut && sfxClick);
}

static void destroySounds(void)
{
    wavplayer_destroy(sfxExplode);
    wavplayer_destroy(sfxPut);
    wavplayer_destroy(sfxClick);
    sfxClick = sfxPut = sfxExplode = NULL;
    wavplayer_stop();
}

static int getBestWindowScale(void)
{
    #ifdef __PSP__
    return 1;
    #else
    SDL_DisplayMode mode;
    if(SDL_GetCurrentDisplayMode(0,&mode) != 0)
        return 2;
    int xscale = mode.w / SCREEN_WIDTH * 0.55f;
    int yscale = mode.h / SCREEN_HEIGHT * 0.55f;
    return SDL_max(SDL_min(xscale,yscale),1);
    #endif
}

static SDL_GameControllerButton mapKeyboardToGamepad(SDL_Keycode key)
{
    switch(key)
    {
        case SDLK_z:
            return SDL_CONTROLLER_BUTTON_A;
        case SDLK_x:
            return SDL_CONTROLLER_BUTTON_B;
        case SDLK_a:
            return SDL_CONTROLLER_BUTTON_X;
        case SDLK_s:
            return SDL_CONTROLLER_BUTTON_Y;
        case SDLK_LEFT:
            return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        case SDLK_RIGHT:
            return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        case SDLK_UP:
            return SDL_CONTROLLER_BUTTON_DPAD_UP;
        case SDLK_DOWN:
            return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case SDLK_RETURN:
            return SDL_CONTROLLER_BUTTON_START;
        default:
            return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

bool game_init(void)
{
    SDL_SetHint(SDL_HINT_APP_NAME, "KleleAtoms");

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        game_errorMsg("Couldn't initialize SDL: %s",SDL_GetError());
        return false;
    }

    char datetime[64];
    getCurrentDate(datetime,64);
    SDL_Log("[%s] Started the game.",datetime);

    int windowScale = getBestWindowScale();

    gameWindow = SDL_CreateWindow("KleleAtoms",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,SCREEN_WIDTH*windowScale,SCREEN_HEIGHT*windowScale,0);
    if(!gameWindow)
    {
        game_errorMsg("Couldn't initialize SDL Window: %s", SDL_GetError());
        return false;
    }

    gameRenderer = SDL_CreateRenderer(gameWindow, -1, SDL_RENDERER_ACCELERATED);
    if(!gameRenderer)
    {
        game_errorMsg("Couldn't initialize SDL Renderer: %s", SDL_GetError());
        return false;
    }

    #ifndef __PSP__
    SDL_RenderSetIntegerScale(gameRenderer, SDL_TRUE);
    SDL_RenderSetLogicalSize(gameRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    #endif

    SDL_SetRenderDrawBlendMode(gameRenderer,SDL_BLENDMODE_BLEND);

    if((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        game_errorMsg("Couldn't initialize SDL Image: %s", SDL_GetError());
        return false;
    }

    if(!assetman_init("resources.pak"))
    {
        game_errorMsg("Couldn't load PAK asset file resources.pak");
        return false;
    }

    if(!assetman_initFont(gameRenderer,"font/DejaVuSans.ttf",14))
    {
        game_errorMsg("Couldn't initialize font resources.pak/font/DejaVuSans.ttf");
        return false;
    }

    if(!initSounds())
    {
        game_errorMsg("Couldn't load sounds!");
        return false;
    }

    initSettings();
    loadSettings();
    clampSettings();

    srand(time(NULL));

    loadStates();
    changeStateInstant(ST_MENUSTATE);

    return true;
}

void game_loop(void)
{
    fade_doFadeIn(0.25f, (SDL_Color){0,0,0,SDL_ALPHA_TRANSPARENT});

    Uint64 last = getTimeTicks();
    Uint64 now = 0;

    bool gameRunning = true;
    SDL_Event event;
    while(gameRunning)
    {
        if(!currentState)
            game_errorMsg("No game state has been set!");

        bool fadeInProgress = fade_isFadeInProgress();
        
        while(SDL_PollEvent(&event))
        {
            SDL_GameControllerButton mappedButton = SDL_CONTROLLER_BUTTON_INVALID;
            switch(event.type)
            {
                case SDL_CONTROLLERBUTTONDOWN:
                    if(currentState->controller_pressed && !fadeInProgress)
                        currentState->controller_pressed(event.cbutton.button, &event);
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    if(currentState->controller_released && !fadeInProgress)
                        currentState->controller_released(event.cbutton.button, &event);
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    SDL_GameControllerOpen(event.cdevice.which);
                    break;
                #ifndef __PSP__
                case SDL_KEYDOWN:
                    mappedButton = mapKeyboardToGamepad(event.key.keysym.sym);
                    if(mappedButton >= 0 && currentState->controller_pressed && !fadeInProgress)
                        currentState->controller_pressed(mappedButton, &event);
                    break;
                case SDL_KEYUP:
                    mappedButton = mapKeyboardToGamepad(event.key.keysym.sym);
                    if(mappedButton >= 0 && currentState->controller_released && !fadeInProgress)
                        currentState->controller_released(mappedButton, &event);
                    break;
                #endif
                case SDL_QUIT:
                    gameRunning = false;
                    return;
                    break;
                default:
                    break;
            }
        }

        now = getTimeTicks();
        float dt = (float)(now-last) / (float)getTickFrequency();

        if(currentState->update && !fadeInProgress)
            currentState->update(dt);

        SDL_SetRenderDrawColor(gameRenderer,0,0,0,SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gameRenderer);
        
        if(currentState->draw)
            currentState->draw(gameRenderer);

        if(fadeInProgress)
            fade_drawFade(gameRenderer, (SDL_Rect){0,0,SCREEN_WIDTH,SCREEN_HEIGHT});

        drawMessage(dt);

        SDL_RenderPresent(gameRenderer);

        last = now;
    }
}

void game_printMsg(const char* str, float time)
{
    messageData.isShown = true;
    messageData.str = str;
    messageData.time = time;
}

void game_errorMsg(const char* format, ...)
{
    char errorBuffer[256];
    va_list args;
    va_start(args,format);
    vsnprintf(errorBuffer, sizeof(errorBuffer), format, args);
    va_end(args);

    SDL_LogError(SDL_LOG_CATEGORY_ERROR,"[ERRORMSG] %s",errorBuffer);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"KleleAtoms PSP Error",errorBuffer,NULL);

    game_quit();
    exit(1);
}

void game_quit(void)
{
    destroySounds();
    rendertext_stop();
    assetman_stop();
    IMG_Quit();
    SDL_DestroyRenderer(gameRenderer);
    SDL_DestroyWindow(gameWindow);
    SDL_Quit();
}
