#include "wavplayer.h"
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <pspaudio.h>

struct WavInfo 
{
    int pspChannel;
    void* pcmData;
    Uint32 pcmSize;
    SDL_AudioSpec spec;
};

static bool setupAudio(WavInfo* audioData, Uint32 sampleCount)
{
    audioData->pspChannel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,sampleCount,(audioData->spec.channels == 2) ? PSP_AUDIO_FORMAT_STEREO : PSP_AUDIO_FORMAT_MONO);
    return audioData->pspChannel != -1;
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
                SDL_RWclose(wav);
                return NULL;
            }
            Uint8* pcmData;
            if(!SDL_LoadWAV_RW(wav,1,&audioData->spec,&pcmData,&audioData->pcmSize))
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Failure to load wav file '%s' \nReason: %s",fileName,SDL_GetError());
                free(audioData);
                return NULL;
            }
            Uint32 alignedSampleCount = PSP_AUDIO_SAMPLE_ALIGN(audioData->pcmSize/(2*audioData->spec.channels));
            Uint32 alignedPcmSize = audioData->pcmSize * (2*audioData->spec.channels);
            audioData->pcmData = calloc(1,alignedPcmSize);
            if(!audioData->pcmData)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Failure to allocate aligned memory for wav file '%s'",fileName);
                SDL_FreeWAV(pcmData);
                free(audioData);
                return NULL;
            }
            memcpy(audioData->pcmData,pcmData,audioData->pcmSize);
            SDL_FreeWAV(pcmData);
            audioData->pcmSize = alignedPcmSize;
            if(!setupAudio(audioData,alignedSampleCount))
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Failure to allocate psp channel for wav file '%s'",fileName);
                free(audioData->pcmData);
                free(audioData);
                return NULL;
            }
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
    if(!audioData)
        return;

    sceAudioOutput(audioData->pspChannel, PSP_AUDIO_VOLUME_MAX, audioData->pcmData);
}

void wavplayer_destroy(WavInfo* audioData)
{
    if(audioData)
    {
        if(audioData->pspChannel)
            sceAudioChRelease(audioData->pspChannel);
        free(audioData->pcmData);
        audioData->pcmData = NULL;
        free(audioData);
    }
}

bool wavplayer_init(void)
{
    return true;
}

void wavplayer_stop(void)
{
    ;
}
