#include "assetman.h"
#include "../utils/pakread.h"
#include <SDL2/SDL_image.h>

//Loaded asset PAK file data
static PakFile* assetPak;

bool assetman_init(const char* pakPath)
{
    assetPak = PAK_OpenFile(pakPath);
    return assetPak != NULL;
}

SDL_Texture* assetman_loadTexture(SDL_Renderer* renderer, const char* assetPath)
{
    SDL_Texture* loadedTex = NULL;
    if(assetPak)
    {
        PakEntryData entry = PAK_LoadEntry(assetPak, assetPath);
        if(entry.data)
        {
            SDL_RWops* stream = SDL_RWFromMem(entry.data, entry.size);
            if(stream)
                loadedTex = IMG_LoadTexture_RW(renderer,stream,1);
        }
        PAK_CloseEntry(&entry);
    }
    return loadedTex;
}

SDL_Surface* assetman_loadSurface(const char* assetPath)
{
    SDL_Surface* loadedImg = NULL;
    if(assetPak)
    {
        PakEntryData entry = PAK_LoadEntry(assetPak, assetPath);
        if(entry.data)
        {
            SDL_RWops* stream = SDL_RWFromMem(entry.data, entry.size);
            if(stream)
                loadedImg = IMG_Load_RW(stream,1);
        }
        PAK_CloseEntry(&entry);
    }
    return loadedImg;
}

PSPWav* assetman_loadWav(const char* assetPath)
{
    PSPWav* loadedWav = NULL;
    if(assetPak)
    {
        PakEntryData entry = PAK_LoadEntry(assetPak, assetPath);
        if(entry.data)
            loadedWav = pspwav_loadFromMem(entry.data, entry.size, assetPath);
        PAK_CloseEntry(&entry);
    }
    return loadedWav;
}

void assetman_stop(void)
{
    if(assetPak)
        PAK_CloseFile(assetPak);
}
