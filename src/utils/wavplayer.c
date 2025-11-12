#include "wavplayer.h"
#include <SDL2/SDL.h>

#ifdef __PSP__
#include <pspaudio.h>
#endif

struct WavInfo 
{
    Uint16 channelCount;
    Uint32 sampleRate;
    Uint16 bitsPerSample;
    int pspChannel;
    void* pcmData;
    size_t pcmSize;
};

// Parses the data chunk, extracts PCM data and aligns the PCM sample count to meet the PSP sample count requirements, returns true on success
static bool parseDataChunk(SDL_RWops* wav, WavInfo* audioData, const char* fileName, Uint32 dataSize)
{
    if(dataSize > 131072)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Audio sizes > 128 KB not supported ('%s')",fileName);
        return false;
    }

    size_t trueDataSize = (((dataSize/2) + 63) & ~63) * 2; //PCM data size in bytes that will be read by the PSP audio subsystem (aligned to 64 samples)
    audioData->pcmData = calloc(trueDataSize,1);
    size_t readSize = SDL_RWread(wav,audioData->pcmData,1,dataSize);
    if(readSize != dataSize)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s': could not read all data",fileName);
        return false;
    }
    audioData->pcmSize = trueDataSize;
    return true;
}

// Parses the format chunk, returns true if parsing was successful
static bool parseFormatChunk(SDL_RWops* wav, WavInfo* audioData, const char* fileName)
{
    Uint16 audioFormat = SDL_ReadLE16(wav);
    if(audioFormat != 1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s': unsupported audio format %d",fileName,audioFormat);
        return false;
    }
    audioData->channelCount = SDL_ReadLE16(wav);
    if(audioData->channelCount == 0 || audioData->channelCount > 2)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s': unsupported channel count %d",fileName,audioData->channelCount);
        return false;
    }
    audioData->sampleRate = SDL_ReadLE32(wav);
    SDL_RWseek(wav,6,RW_SEEK_CUR); //Skip BytesPerSecond (4 byte) and BytePerBloc (2 byte) values
    audioData->bitsPerSample = SDL_ReadLE16(wav);
    return true;
}

/// @brief Parses the WAV file data
/// @param wav WAV file data IO stream
/// @param wavSize WAV file data size
/// @param fileName WAV file name, used for error log messages
/// @return WavInfo audio data struct for use in wavplayer_play (or NULL on failure)
static WavInfo* parseWavFile(SDL_RWops* wav, size_t wavSize, const char* fileName)
{
    WavInfo* audioData = calloc(1,sizeof(WavInfo));
    audioData->pspChannel = -1;
    char idstr[5] = {'\0'};
    //RIFF identifier check
    SDL_RWread(wav,idstr,sizeof(char),4);
    if(strcmp(idstr,"RIFF") != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s' is invalid or corrupted",fileName);
        wavplayer_destroy(audioData);
        return NULL;
    }
    //True File size check
    Uint32 trueFileSize = SDL_ReadLE32(wav) + 8;
    if(trueFileSize < wavSize)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s' is too small",fileName);
        wavplayer_destroy(audioData);
        return NULL;
    }
    //Wave format check
    SDL_RWread(wav,idstr,sizeof(char),4);
    if(strcmp(idstr,"WAVE") != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav file '%s' is not a valid wav file",fileName);
        wavplayer_destroy(audioData);
        return NULL;
    }
    bool formatParsed = false;
    bool dataParsed = false;
    //Parse chunks
    while(SDL_RWtell(wav) < trueFileSize)
    {
        SDL_RWread(wav,idstr,sizeof(char),4);
        Uint32 chunkSize = SDL_ReadLE32(wav);
        if(strcmp(idstr,"fmt ") == 0) //Data format chunk
        {
            if(chunkSize != 16)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav Data format chunk size is invalid in file %s (%d)",fileName,chunkSize);
                wavplayer_destroy(audioData);
                return NULL;
            }
            formatParsed = parseFormatChunk(wav,audioData,fileName);
        }
        else if(strcmp(idstr,"data") == 0) //Data chunk
        {
            if((SDL_RWtell(wav) + chunkSize) > wavSize)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav Data extends beyond the file size in file %s",fileName);
                wavplayer_destroy(audioData);
                return NULL;
            }
            dataParsed = parseDataChunk(wav,audioData,fileName,chunkSize);
        }
        else //Unsupported chunk, skip it
        {
            SDL_RWseek(wav,chunkSize,RW_SEEK_CUR);
        }
    }
    if(!formatParsed)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav Data format not parsed in file %s",fileName);
        wavplayer_destroy(audioData);
        return NULL;
    }
    if(!dataParsed)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Wav Data not parsed in file %s",fileName);
        wavplayer_destroy(audioData);
        return NULL;
    }
    return audioData;
}

/// @brief Loads a WAV file from memory (internal function used by both wavplayer_load functions)
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
            WavInfo* audioData = parseWavFile(wav,wavSize,fileName);
            SDL_RWclose(wav);
            if(!audioData)
                return NULL;
            #ifdef __PSP__
            audioData->pspChannel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, PSP_AUDIO_SAMPLE_ALIGN(audioData->pcmSize/2), (audioData->channelCount == 2) ? PSP_AUDIO_FORMAT_STEREO : PSP_AUDIO_FORMAT_MONO);
            if(audioData->pspChannel < 0)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,"wavplayer: Failure to initialize wav channel for '%s' (error code %d)",fileName,audioData->pspChannel);
                wavplayer_destroy(audioData);
                return NULL;
            }
            #endif
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

    #ifdef __PSP__
    sceAudioOutput(audioData->pspChannel, PSP_AUDIO_VOLUME_MAX, audioData->pcmData);
    #endif
}

void wavplayer_destroy(WavInfo* audioData)
{
    if(audioData)
    {
        #ifdef __PSP__
        if(audioData->pspChannel >= 0)
            sceAudioChRelease(audioData->pspChannel);
        #endif
        free(audioData->pcmData);
        audioData->pcmData = NULL;
        free(audioData);
    }
}
