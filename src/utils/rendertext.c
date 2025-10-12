#include "rendertext.h"
#include <SDL2/SDL.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#define CHAR_AMOUNT 128
#define SCREEN_WIDTH 480

static stbtt_packedchar packedChars[CHAR_AMOUNT];
static SDL_Texture* fontAtlas;
static SDL_Renderer* renderer;
static float fontHeight;
enum TextAlignment textAlign;
int drawWidth;

// Get the SDL window width
static int getWindowWidth(SDL_Renderer* rend)
{
    SDL_Window* win = SDL_RenderGetWindow(rend);
    int w;
    SDL_GetWindowSize(win,&w,NULL);
    return w;
}

/// @brief Create a font atlas texture from stb_truetype packed font
/// @param rend SDL_Renderer to use for the texture
/// @param fontPixels Packed font pixel data from stb_truetype (array of width * height alpha bytes)
/// @return Font atlas texture on success, NULL on failure
static SDL_Texture* createTextureFont(SDL_Renderer* rend, unsigned char* fontPixels)
{
    SDL_Surface* fontSurface = SDL_CreateRGBSurfaceWithFormat(0,FONT_PIXEL_ARRAY_SIZE,FONT_PIXEL_ARRAY_SIZE,32,SDL_PIXELFORMAT_ABGR8888);
    if(fontSurface)
    {
        SDL_LockSurface(fontSurface);
        Uint32* pixels = (Uint32*)fontSurface->pixels;
        for(int i=0; i<FONT_PIXEL_ARRAY_SIZE*FONT_PIXEL_ARRAY_SIZE; i++)
        {
            pixels[i] = SDL_MapRGBA(fontSurface->format, 255, 255, 255, fontPixels[i]);
        }
        pixels = NULL;
        SDL_UnlockSurface(fontSurface);
        SDL_Texture* newTex = SDL_CreateTextureFromSurface(rend, fontSurface);
        SDL_FreeSurface(fontSurface);
        SDL_SetTextureBlendMode(newTex, SDL_BLENDMODE_BLEND);
        return newTex;
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"rendertext_init: creating atlas surface failed!");
        return NULL;
    }
}

bool rendertext_init(SDL_Renderer* rend, const char* fontFileName, float height)
{
    if(fontAtlas)
        rendertext_stop();

    textAlign = TEXT_ALIGN_LEFT;
    drawWidth = getWindowWidth(rend);
    size_t fontDataSize;
    unsigned char* fontData = SDL_LoadFile(fontFileName,&fontDataSize);
    if(fontData)
    {
        stbtt_fontinfo font;
        stbtt_pack_context pack;
        unsigned char* fontPixels = calloc(FONT_PIXEL_ARRAY_SIZE*FONT_PIXEL_ARRAY_SIZE,sizeof(unsigned char));
        if(!fontPixels)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "rendertext_init: Couldn't allocate font pixel data");
            SDL_free(fontData);
            return false;
        }
        
        stbtt_InitFont(&font, fontData, 0);
        stbtt_PackBegin(&pack, fontPixels, FONT_PIXEL_ARRAY_SIZE, FONT_PIXEL_ARRAY_SIZE, 0, 1, NULL);
        stbtt_PackSetOversampling(&pack, FONT_OVERSAMPLING, FONT_OVERSAMPLING);
        if(!stbtt_PackFontRange(&pack, fontData, 0, STBTT_POINT_SIZE(height), 1, CHAR_AMOUNT-1, packedChars))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "rendertext_init: FONT_PIXEL_ARRAY_SIZE is too small for the given font size + oversampling settings");
            SDL_free(fontData);
            free(fontPixels);
            stbtt_PackEnd(&pack);
            return false;
        }
        stbtt_PackEnd(&pack);

        fontAtlas = createTextureFont(rend,fontPixels);
        SDL_free(fontData);
        free(fontPixels);
        if(!fontAtlas)
            return false;
        fontHeight = height;
        renderer = rend;
        return true;
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "rendertext_init: Couldn't load the font!");
        return false;
    }
}

/// @brief Get a width of a text line
/// @param str String pointer starting with the line to check
/// @return Line width in pixels
static int getLineWidth(const char* str)
{
    int curx = 0;
    int width = 0;
    while(*str && *str != '\n')
    {
        const char c = *str;
        if((unsigned char)c <= 127)
        {
            curx += packedChars[c-1].xadvance;
        }
        str++;
    }
    return curx;
}

/// @brief Get the X position to start rendering text at based on current alignment, base x position and width
/// @param str String that's going to be rendered
/// @param x Base X position given to rendertext_drawTextColored function
/// @param textWidth Text width in pixels (only applies to RIGHT and CENTER alignments)
/// @return The X position text rendering should start at
static int getAlignmentX(const char* str, int x, int textWidth)
{
    switch(textAlign)
    {
        case TEXT_ALIGN_RIGHT:
            return x + (textWidth - getLineWidth(str));
        case TEXT_ALIGN_CENTER:
            return x + ((textWidth - getLineWidth(str)) / 2);
        case TEXT_ALIGN_LEFT:
        default:
            return x;
    }
}

void rendertext_drawTextColored(const char* str, int x, int y, SDL_Color color)
{
    if(!fontAtlas)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"rendertext_drawTextColored: font has not been loaded!");
        return;
    }
    SDL_SetTextureColorMod(fontAtlas,color.r,color.g,color.b);
    SDL_SetTextureAlphaMod(fontAtlas,color.a);
    int curx = getAlignmentX(str,x,drawWidth);
    int cury = y+fontHeight-2;
    while(*str)
    {
        const char c = *str;
        if(c == '\n')
        {
            curx = getAlignmentX(str+1,x,drawWidth);
            cury += fontHeight + 2;
        }
        else if((unsigned char)c <= 127)
        {
            stbtt_packedchar* cbounds = &packedChars[c-1];
            float gWidth = cbounds->x1 - cbounds->x0;
            float gHeight = cbounds->y1 - cbounds->y0;
            SDL_Rect sourceRect = {cbounds->x0,cbounds->y0,gWidth,gHeight};
            SDL_FRect destRect = {curx+cbounds->xoff,cury+cbounds->yoff,cbounds->xoff2-cbounds->xoff,cbounds->yoff2-cbounds->yoff};
            SDL_RenderCopyF(renderer,fontAtlas,&sourceRect,&destRect);
            curx += cbounds->xadvance;
        }
        str++;
    }
}

inline void rendertext_drawText(const char* str, int x, int y)
{
    rendertext_drawTextColored(str,x,y,(SDL_Color){255,255,255,SDL_ALPHA_OPAQUE});
}

enum TextAlignment rendertext_getTextAlignment(int* textWidth)
{
    if(textWidth)
        *textWidth = drawWidth;
    return textAlign;
}

void rendertext_setTextAlignment(enum TextAlignment align, int maxTextWidth)
{
    if(renderer && maxTextWidth < 0)
        drawWidth = getWindowWidth(renderer);
    else
        drawWidth = maxTextWidth;
    textAlign = align;
}

SDL_Texture* rendertext_getAtlas(void)
{
    return fontAtlas;
}

void rendertext_stop(void)
{
    renderer = NULL;
    if(fontAtlas)
    {
        SDL_DestroyTexture(fontAtlas);
        fontAtlas = NULL;
    }
}
