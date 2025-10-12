#pragma once
#include <SDL2/SDL.h>

/// @brief Initializes the game assets and draw positions
/// @param gridWidth Grid width
/// @param gridHeight Grid height
void gamedraw_initAssets(int gridWidth, int gridHeight);

// Draws the grid on the screen
void gamedraw_drawGrid(void);

// Draws all the visible atoms
void gamedraw_drawAtoms(void);

/// @brief Draws player status icons and the timer
/// @param gameTime Current game time in seconds to use for the timer
void gamedraw_drawHUD(Sint32 gameTime);

/// @brief Draws the tile selector
/// @param x Selected tile X position
/// @param y Selected tile Y position
void gamedraw_drawSelector(int x, int y);

// Draws the pause window
void gamedraw_drawPauseWindow(void);

/// @brief Draws the victory window
/// @param gameTime Victory game time, only used the first time after calling this function in a given session
void gamedraw_drawVictoryWindow(Sint32 gameTime);

// Unloads the game assets
void gamedraw_destroyAssets(void);
