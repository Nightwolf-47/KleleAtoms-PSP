#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../utils/pspwav.h"

/// @brief Initalize asset manager by giving it the path of a PAK file
/// @param pakPath PAK asset file path
/// @return true on success, false on failure
bool assetman_init(const char* pakPath);

/// @brief Get an image from loaded PAK file as an SDL surface
/// @param assetPath Image path inside the PAK file
/// @return Image surface on success, NULL on failure
SDL_Surface* assetman_loadSurface(const char* assetPath);

/// @brief Get an image from loaded PAK file as an SDL texture
/// @param renderer SDL Renderer used for the texture
/// @param assetPath Image path inside the PAK file
/// @return Image texture on success, NULL on failure
SDL_Texture* assetman_loadTexture(SDL_Renderer* renderer, const char* assetPath);

/// @brief Load a WAV file from loaded PAK file
/// @param assetPath WAV file path inside the PAK file
/// @return Sound data for use in pspwav on success, NULL on failure
PSPWav* assetman_loadWav(const char* assetPath);

// Closes the PAK file and stops the asset manager
void assetman_stop(void);
