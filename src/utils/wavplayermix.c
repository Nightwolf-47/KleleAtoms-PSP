#include "wavplayer.h"
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

struct WavInfo 
{
    Mix_Chunk* audio;
    int channel;
};

static int getNewChannel(void)
{
    static int channel = 0;
    if(channel >= MIX_CHANNELS)
        return -1;
    return channel++;
}

/// @brief Loads a WAV file from memory
/// @param wavData Memory buffer containing WAV file data
/// @param wavSize Size of WAV data buffer
/// @param wavName WAV file name, used for error log messages
/// @return WavInfo audio data struct for use in wavplayer_play (or NULL on failure)
static WavInfo* loadWavData(void* wavData, size_t wavSize, const char* fileName)
{
    if(wavSize < 44)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s' is too small",fileName);
        return NULL;
    }

    if(wavData)
    {
        SDL_RWops* wav = SDL_RWFromConstMem(wavData,wavSize);
        if(wav)
        {
            WavInfo* audioData = calloc(1,sizeof(WavInfo));
            if(!audioData)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Failure to allocate wav data memory");
                return NULL;
            }
            audioData->audio = Mix_LoadWAV_RW(wav,1);
            if(!audioData->audio)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Failure to load wav file '%s' \nReason: %s",fileName,SDL_GetError());
                free(audioData);
                return NULL;
            }
            audioData->channel = getNewChannel();
            return audioData;
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Could not parse loaded wav file '%s'",fileName);
            return NULL;
        }
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Could not load wav file '%s'",fileName);
        return NULL;
    }
}

WavInfo* wavplayer_load(const char* fileName)
{
    size_t wavSize;
    void* wavData = SDL_LoadFile(fileName,&wavSize);
    WavInfo* audioData = loadWavData(wavData,wavSize,fileName);
    SDL_free(wavData);
    return audioData;
}

WavInfo* wavplayer_loadFromMem(void* wavData, size_t wavSize, const char* wavName)
{
    return loadWavData(wavData,wavSize,wavName);
}

void wavplayer_play(WavInfo* audioData)
{
    if(!audioData || !audioData->audio)
        return;

    Mix_PlayChannel(audioData->channel, audioData->audio, 0);
}

void wavplayer_destroy(WavInfo* audioData)
{
    if(audioData)
    {
        if(audioData->audio)
            Mix_FreeChunk(audioData->audio);
        audioData->audio = NULL;
        free(audioData);
    }
}

bool wavplayer_init(void)
{
    if(Mix_OpenAudio(44100,AUDIO_S16,2,2048) == -1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Could initialize SDL_Mixer, Reason: %s",SDL_GetError());
        return false;
    }
    return true;
}

void wavplayer_stop(void)
{
    Mix_CloseAudio();
}
