#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct PakEntryData {
    size_t size; // Size of the data in bytes
    void* data; // Entry data, it can be NULL if the entry didn't load (entry wasn't found or data couldn't be allocated) or this entry was closed
} PakEntryData;

// PAK File data used with the PAK functions
typedef struct PakFile PakFile;

/// @brief Opens the PAK file for reading.
/// @param pakName PAK file path
/// @return On success, Structure containing PAK data info, NULL on failure
PakFile* PAK_OpenFile(const char* pakName);

/// @brief Closes the PAK file and frees its memory.
/// @param file PAK file to close
void PAK_CloseFile(PakFile* file);

/// @brief Loads the PAK entry data and returns its data + size to the PakEntryData struct. If the data variable isn't NULL, you have to use PAK_CloseEntry on the returned struct.
/// @param file PAK file struct
/// @param entryPath Full PAK entry name/path
/// @return Struct containing data and size on success or struct with .data == NULL on failure
PakEntryData PAK_LoadEntry(PakFile* file, const char* entryPath);

/// @brief Closes the loaded PAK entry and frees the entry data, can be used even if data is NULL (it does nothing in that case).
/// @param loadedEntry PAK entry returned by PAK_LoadEntry function
void PAK_CloseEntry(PakEntryData* loadedEntry);
