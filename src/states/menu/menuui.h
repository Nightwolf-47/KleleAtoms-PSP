#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>

/// @brief Initialize the UI
/// @param renderer SDL Renderer to use for the UI
/// @return true if init was a success, false otherwise
bool menuui_init(SDL_Renderer* renderer);

// Update the menu UI (react to continuous button presses)
void menuui_update(void);

/// @brief Draw the menu UI
/// @param renderer SDL Renderer to draw to
void menuui_draw(SDL_Renderer* renderer);

/// @brief React to a pressed button
/// @param pressedButton Pressed controller button
void menuui_press(SDL_GameControllerButton pressedButton);

/// @brief React to a released button
/// @param pressedButton Released controller button
void menuui_release(SDL_GameControllerButton releasedButton);

// Stops the UI and unloads all UI assets
void menuui_stop(void);
