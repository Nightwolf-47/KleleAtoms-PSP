#include "pakread.h"
#include <stdio.h>
#include <SDL2/SDL.h>

typedef struct PakEntry {
    char name[56];
    int32_t offset;
    int32_t size;
} PakEntry;

typedef struct PakFile {
    PakEntry* entries;
    int32_t entryCount;
    FILE* stream;
} PakFile;

// Reads a 32-bit integer from file (requires little endian)
static uint32_t readInt32(FILE* stream)
{
    uint32_t result;
    if(fread(&result, 1, sizeof(uint32_t), stream) != sizeof(uint32_t))
        return UINT32_MAX;
    return result;
}

// Returns the opened file size
static size_t getFileSize(FILE* stream)
{
    long lastPos = ftell(stream);
    fseek(stream,0,SEEK_END);
    long size = ftell(stream);
    fseek(stream,lastPos,SEEK_SET);
    return size;
}

// Parses the PAK file: reads the entries data and validates it
static bool parsePakFile(PakFile* file, FILE* stream)
{
    if(!file)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: Couldn't allocate the PakFile struct.");
        return false;
    }
    file->stream = stream;

    size_t fileSize = getFileSize(stream);
    if(fileSize < 12)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: File size is not big enough to contain the PAK header (%u < 12)",fileSize);
        return false;
    }
    fseek(stream, 0, SEEK_SET);
    char magicStr[5] = {'\0'};
    fread(magicStr, sizeof(char), 4, stream);
    if(strcmp(magicStr,"PACK") != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: Header string '%s' != 'PACK'",magicStr);
        return false;
    }
    uint32_t dirOffset = readInt32(stream);
    uint32_t dirSize = readInt32(stream);
    if(dirOffset==UINT32_MAX || dirSize==UINT32_MAX || (uint64_t)dirOffset+dirSize > fileSize)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: directory offset + size > filesize (%u+%u > %u)",dirOffset,dirSize,fileSize);
        return false;
    }
    uint32_t entryCount = dirSize / 64;
    file->entryCount = entryCount;
    file->entries = calloc(entryCount,sizeof(PakEntry));
    if(!file->entries)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: couldn't allocate the entry data!");
        return false;
    }
    fseek(stream,dirOffset,SEEK_SET);
    for(uint32_t i=0; i<entryCount; i++)
    {
        size_t read = fread(file->entries[i].name,sizeof(char),56,stream);
        file->entries[i].name[55] = '\0';  //The name string has to be NULL terminated
        uint32_t offset = readInt32(stream);
        uint32_t size = readInt32(stream);
        if((uint64_t)offset+size > fileSize)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: offset + size > fileSize (%u+%u > %u)",offset,size,fileSize);
            return false;
        }
        file->entries[i].size = size;
        file->entries[i].offset = offset;
    }
    return true;
}

PakFile* PAK_OpenFile(const char* fileName)
{
    FILE* fileStream = fopen(fileName, "rb");
    if(fileStream)
    {
        PakFile* file = malloc(sizeof(PakFile));
        if(parsePakFile(file,fileStream))
        {
            return file;
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"PAK_OpenFile: Couldn't parse the PAK file %s", fileName);
            fclose(fileStream);
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

void PAK_CloseFile(PakFile* file)
{
    if(file)
    {
        if(file->stream)
            fclose(file->stream);
        file->stream = NULL;
        free(file->entries);
        file->entries = NULL;
    }
}

/// @brief Search for a PAK entry with a given path
/// @param file PAK file to search in
/// @param entryPath PAK entry path
/// @return PakEntry data for a found entry or NULL if the entry couldn't be found
static PakEntry* getPackEntry(PakFile *file, const char *entryPath)
{
    if(file && file->entries && entryPath)
    {
        for(int32_t i=0; i<file->entryCount; i++)
        {
            if(strcmp(file->entries[i].name,entryPath) == 0)
                return &file->entries[i];
        }
    }
    return NULL;
}

PakEntryData PAK_LoadEntry(PakFile* file, const char* entryPath)
{
    PakEntry* entry = getPackEntry(file,entryPath);

    if(!file || !entry || entry->size < 0 || !file->stream)
        return (PakEntryData){.size = -1, .data = NULL};

    void* data = malloc(SDL_max(entry->size,1)*sizeof(char));
    if(!data)
        return (PakEntryData){.size = -1, .data = NULL};
    fseek(file->stream, entry->offset, SEEK_SET);
    fread(data, entry->size, 1, file->stream);
    return (PakEntryData){.size = entry->size, .data = data};
}

void PAK_CloseEntry(PakEntryData *loadedEntry)
{
    if(loadedEntry && loadedEntry->data)
    {
        free(loadedEntry->data);
        loadedEntry->data = NULL;
    }
}
