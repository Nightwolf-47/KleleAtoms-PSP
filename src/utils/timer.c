#include "timer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

struct KTimer {
    Sint64 startTime;
};

KTimer* ktimer_create()
{
    KTimer* newTimer = (KTimer*)malloc(sizeof(KTimer));
    newTimer->startTime = SDL_GetTicks64();
    return newTimer;
}

Sint64 ktimer_getTimeMillis(KTimer *timer)
{
    if(!timer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"ktimer_getTimeMillis: timer is null");
        return INT64_MIN;
    }

    return (Sint64)SDL_GetTicks64() - timer->startTime;
}

Sint64 ktimer_getTimeSeconds(KTimer *timer)
{
    if(!timer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"ktimer_getTimeSeconds: timer is null");
        return INT64_MIN;
    }

    return ((Sint64)SDL_GetTicks64() - timer->startTime) / 1000;
}

float ktimer_getTimeFloat(KTimer *timer)
{
    if(!timer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"ktimer_getTimeFloat: timer is null");
        return INFINITY;
    }

    return ((Sint64)SDL_GetTicks64() - timer->startTime) / 1000.0f;
}

void ktimer_setTimeMillis(KTimer *timer, Sint64 timeMillis)
{
    if(!timer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"ktimer_setTimeMillis: timer is null");
        return;
    }

    timer->startTime = (Sint64)SDL_GetTicks64() - timeMillis;
}

void ktimer_setTimeSeconds(KTimer *timer, Sint64 timeSeconds)
{
    if(!timer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"ktimer_setTimeMillis: timer is null");
        return;
    }

    timer->startTime = (Sint64)SDL_GetTicks64() - (timeSeconds*1000);
}

void ktimer_setTimeFloat(KTimer *timer, float time)
{
    if(!timer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"ktimer_setTimeFloat: timer is null");
        return;
    }
    
    timer->startTime = (Sint64)SDL_GetTicks64() - ((Sint64)ceilf(time/1000));
}

void ktimer_destroy(KTimer *timer)
{
    free(timer);
}
