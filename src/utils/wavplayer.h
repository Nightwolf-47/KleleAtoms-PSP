#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct WavInfo WavInfo;

/// @brief Load a WAV file
/// @param fileName WAV file path (must not be bigger than ~128 KB)
/// @return WavInfo audio data struct for use in wavplayer_play (or NULL on failure)
WavInfo* wavplayer_load(const char* fileName);

/// @brief Load a WAV file from memory
/// @param wavData Memory buffer containing WAV file data
/// @param wavSize Size of WAV data buffer
/// @param wavName WAV file name, used for error log messages
/// @return WavInfo audio data struct for use in wavplayer_play (or NULL on failure)
WavInfo* wavplayer_loadFromMem(void* wavData, size_t wavSize, const char* wavName);

/// @brief Play a sound
/// @param audioData Loaded sound audio data
void wavplayer_play(WavInfo* audioData);

/// @brief Deallocate sound memory and remove sound
/// @param audioData Sound audio data to destroy
void wavplayer_destroy(WavInfo* audioData);
