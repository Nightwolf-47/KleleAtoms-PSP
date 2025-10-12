#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct PSPWav PSPWav;

/// @brief Load a WAV file
/// @param fileName WAV file path (must not be bigger than ~128 KB)
/// @return PSPWav audio data struct for use in pspwav_play (or NULL on failure)
PSPWav* pspwav_load(const char* fileName);

/// @brief Load a WAV file from memory
/// @param wavData Memory buffer containing WAV file data
/// @param wavSize Size of WAV data buffer
/// @param wavName WAV file name, used for error log messages
/// @return PSPWav audio data struct for use in pspwav_play (or NULL on failure)
PSPWav* pspwav_loadFromMem(void* wavData, size_t wavSize, const char* wavName);

/// @brief Play a sound
/// @param audioData Loaded sound audio data
void pspwav_play(PSPWav* audioData);

/// @brief Deallocate sound memory and remove sound
/// @param audioData Sound audio data to destroy
void pspwav_destroy(PSPWav* audioData);
