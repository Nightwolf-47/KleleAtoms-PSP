#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>

#define FONT_PIXEL_ARRAY_SIZE 256
#define FONT_OVERSAMPLING 2

enum TextAlignment {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
};

/// @brief Load a font for use with rendertext_* functions
/// @param rend SDL Renderer to draw to
/// @param fontFileName Path to the TTF font file
/// @param height Font height in pixels
/// @return true on success, false on failure
bool rendertext_init(SDL_Renderer* rend, const char* fontFileName, float height);

/// @brief Draw text on the screen with a given color (supports multiline strings)
///
/// On TEXT_ALIGN_RIGHT align mode, the X position is relative to the right side instead of the left side.
///
/// @param str String to draw
/// @param x Text X position
/// @param y Text Y position
/// @param color Text color to use when drawing the string
void rendertext_drawTextColored(const char* str, int x, int y, SDL_Color color);

/// @brief Draw text on the screen (supports multiline strings)
/// @param str String to draw
/// @param x Text X position
/// @param y Text Y position
void rendertext_drawText(const char* str, int x, int y);

/// @brief Returns the current text alignment enum and puts the current text width in the textWidth pointer if it's not NULL.
/// @param textWidth Optional pointer to put the text width to
/// @return Current TextAlignment enum 
enum TextAlignment rendertext_getTextAlignment(int* textWidth);

/// @brief Set text alignment (TEXT_ALIGN_LEFT resets the alignment)
///
/// WARNING: Drawn text may go beyond textWidth, the value is only for adjusting alignment.
///
/// @param align text alignment mode (TEXT_ALIGN_* enums)
/// @param textWidth text "bounding box" width for TEXT_ALIGN_CENTER and TEXT_ALIGN_RIGHT modes, setting it to -1 sets textWidth to window width
void rendertext_setTextAlignment(enum TextAlignment align, int textWidth);

// Returns the raw text atlas texture used by rendertext_* functions
SDL_Texture* rendertext_getAtlas(void);

// Frees up the resources used by rendertext
void rendertext_stop(void);
