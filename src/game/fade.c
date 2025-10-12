#include "fade.h"
#include "../utils/timer.h"

typedef struct FadeColorDelta {
    int r;
    int g;
    int b;
    int a;
} FadeColorDelta;

struct FadeEffect {
    bool inProgress;
    SDL_Color baseColor;
    FadeColorDelta colorDelta;
    KTimer* fadeTimer;
    float fadeMultiplier;
    void (*fadeEndCallback)(void);
};

struct FadeEffect fadeInfo = {.inProgress = false};

void fade_doFade(float fadeTime, SDL_Color startColor, SDL_Color targetColor)
{
    if(fadeInfo.inProgress)
        fade_stopFade();

    fadeInfo.fadeTimer = ktimer_create();
    fadeInfo.fadeMultiplier = 1.0f/fadeTime;
    fadeInfo.inProgress = true;
    fadeInfo.baseColor = startColor;
    fadeInfo.colorDelta.r = (int)targetColor.r - (int)startColor.r;
    fadeInfo.colorDelta.g = (int)targetColor.g - (int)startColor.g;
    fadeInfo.colorDelta.b = (int)targetColor.b - (int)startColor.b;
    fadeInfo.colorDelta.a = (int)targetColor.a - (int)startColor.a;
}

void fade_doFadeOut(float fadeTime, SDL_Color targetColor)
{
    SDL_Color startColor = targetColor;
    startColor.a = SDL_ALPHA_TRANSPARENT;
    fade_doFade(fadeTime, startColor, targetColor);
}

void fade_doFadeIn(float fadeTime, SDL_Color targetColor)
{
    SDL_Color startColor = targetColor;
    startColor.a = SDL_ALPHA_OPAQUE;
    fade_doFade(fadeTime, startColor, targetColor);
}

void fade_setFadeEndCallback(void (*fadeEndCallback)(void))
{
    fadeInfo.fadeEndCallback = fadeEndCallback;
}

void fade_drawFade(SDL_Renderer* renderer, SDL_Rect fadeRect)
{
    if(fadeInfo.inProgress && fadeInfo.fadeTimer && fadeInfo.fadeMultiplier > 0)
    {
        float progress = ktimer_getTimeFloat(fadeInfo.fadeTimer) * fadeInfo.fadeMultiplier;
        progress = SDL_min(progress, 1.0f);
        SDL_Color oldColor;
        SDL_GetRenderDrawColor(renderer, &oldColor.r, &oldColor.g, &oldColor.b, &oldColor.a);
        SDL_Color fadeColor = {
            fadeInfo.baseColor.r + (fadeInfo.colorDelta.r * progress),
            fadeInfo.baseColor.g + (fadeInfo.colorDelta.g * progress),
            fadeInfo.baseColor.b + (fadeInfo.colorDelta.b * progress),
            fadeInfo.baseColor.a + (fadeInfo.colorDelta.a * progress),
        };
        SDL_SetRenderDrawColor(renderer, fadeColor.r, fadeColor.g, fadeColor.b, fadeColor.a);
        SDL_RenderFillRect(renderer, &fadeRect);
        SDL_SetRenderDrawColor(renderer, oldColor.r, oldColor.g, oldColor.b, oldColor.a);
        if(progress >= 1.0f)
        {
            if(fadeInfo.fadeEndCallback)
                fadeInfo.fadeEndCallback();
            else
                fade_stopFade();
        }
    }
}

bool fade_isFadeInProgress(void)
{
    return fadeInfo.inProgress;
}

void fade_stopFade(void)
{
    fadeInfo.inProgress = false;
    fadeInfo.fadeMultiplier = 0;
    ktimer_destroy(fadeInfo.fadeTimer);
}
