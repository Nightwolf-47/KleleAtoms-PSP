#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

/// @brief Start a fade animation going linearly from startColor to targetColor
/// @param fadeTime Fade time in seconds
/// @param startColor Start fade color
/// @param targetColor Target fade color
void fade_doFade(float fadeTime, SDL_Color startColor, SDL_Color targetColor);

/// @brief Start a fade out animation (increases alpha value from 0 until it reaches targetColor alpha)
/// @param fadeTime Fade time in seconds
/// @param targetColor Target fade color
void fade_doFadeOut(float fadeTime, SDL_Color targetColor);

/// @brief Start a fade out animation (decreases alpha value from 255 until it reaches targetColor alpha)
/// @param fadeTime Fade time in seconds
/// @param targetColor Target fade color
void fade_doFadeIn(float fadeTime, SDL_Color targetColor);

/// @brief Set a callback function to call when a fade animation ends. If there's no callback, the animation stops instantly after finishing.
/// @param fadeEndCallback New fade end callback function or NULL to remove the previous callback
void fade_setFadeEndCallback(void (*fadeEndCallback)(void));

/// @brief Draws a fade animation (should be called once per frame after drawing other things, but before SDL_RenderPresent)
/// @param renderer SDL Renderer to draw to
/// @param fadeRect Size and position of the fade rectangle
void fade_drawFade(SDL_Renderer* renderer, SDL_Rect fadeRect);

/// @brief Checks if fade animation is in progress
/// @return true if there's a fade animation is progress, false otherwise
bool fade_isFadeInProgress(void);

/// @brief Stop a fade animation (required when a callback is set, otherwise only needed to stop it prematurely)
void fade_stopFade(void);
