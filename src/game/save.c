#include "save.h"
#include "state.h"
#include "game.h"
#include "../utils/timer.h"
#include "../states/game/gamestate.h"
#include "../states/game/gamelogic.h"
#include "../states/game/gameai.h"
#include <SDL2/SDL.h>

#ifdef _MSC_VER
#include <io.h>
#define access(path,mode) _access(path,mode)
#define F_OK 0
#else
#include <unistd.h>
#endif

// KSF Save file header size
#define SAVE_HEADER_SIZE 20

// File path to the settings file
const char* settingsFilePath = "settings.dat";
// File path to the KSF save file
const char* saveFilePath = "savegame.ksf";

// KSF save file magic number
const char* saveMagicNum = "KSF";

void loadSettings(void)
{
    SDL_RWops* file = SDL_RWFromFile(settingsFilePath,"rb");
    if(file)
    {
        gameSettings.gridWidth = SDL_ReadU8(file);
        gameSettings.gridHeight = SDL_ReadU8(file);
        gameSettings.player1Type = SDL_ReadU8(file);
        gameSettings.player2Type = SDL_ReadU8(file);
        gameSettings.player3Type = SDL_ReadU8(file);
        gameSettings.player4Type = SDL_ReadU8(file);
        SDL_RWclose(file);
    }
}

void saveSettings(void)
{
    SDL_RWops* file = SDL_RWFromFile(settingsFilePath,"wb");
    if(file)
    {
        SDL_WriteU8(file, gameSettings.gridWidth & 0xFF);
        SDL_WriteU8(file, gameSettings.gridHeight & 0xFF);
        SDL_WriteU8(file, gameSettings.player1Type & 0xFF);
        SDL_WriteU8(file, gameSettings.player2Type & 0xFF);
        SDL_WriteU8(file, gameSettings.player3Type & 0xFF);
        SDL_WriteU8(file, gameSettings.player4Type & 0xFF);
        SDL_RWclose(file);
    }
}

bool isSavePresent(void)
{
    return (access(saveFilePath,F_OK) == 0);
}

/// @brief Closes and removes a loaded save file (done at the end of loading or after a load error)
/// @param file KSF load file stream
static void closeLoadedSaveFile(SDL_RWops* file)
{
    SDL_RWclose(file);
    remove(saveFilePath);
}

/// @brief Converts the KSF player status value to internal data and loads it
/// @param playerNum Player number (0-3)
/// @param playerStatus Player status byte loaded from KSF save file
static void loadPlayerStatus(int playerNum, Uint8 playerStatus)
{
    switch(playerStatus)
    {
        case 0:
            logicData->playerStatus[playerNum] = PST_LOST;
            logicData->playerAtoms[playerNum] = 0;
            break;
        case 1:
            logicData->playerStatus[playerNum] = PST_NOTSTARTED;
            logicData->playerAtoms[playerNum] = 0;
            break;
        case 2:
            logicData->playerStatus[playerNum] = PST_PLAYING;
            logicData->playerAtoms[playerNum] = 1;
            break;
        default:
            logicData->playerStatus[playerNum] = PST_NOTPRESENT;
            logicData->playerAtoms[playerNum] = 0;
            break;
    }
}

/// @brief Converts the KSF AI Type value to internal data and loads it
/// @param playerNum Player number (0-3)
/// @param val AI Type byte loaded from KSF save file
/// @param difficulty Default AI difficulty used when val == 1
static void loadAIType(int playerNum, Uint8 val, Uint8 difficulty)
{
    aiDifficulty[playerNum] = 0;
    val = SDL_min(val,4);
    if(val == 0)
    {
        aiPlayer[playerNum] = false;
    }
    else
    {
        aiPlayer[playerNum] = true;
        if(val == 1)
            aiDifficulty[playerNum] = difficulty;
        else
            aiDifficulty[playerNum] = val - 1;
    }
}

int loadGame(void)
{
    // Player numbers loaded directly from KSF have to be lowered by 1 due to the format being designed for original KleleAtoms, which was made in Lua, a language with 1-indexed arrays.
    // (Specifically the current player and tile player numbers)

    if(!logicData)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"loadGame: Tried to load game before game init!");
        return -1;
    }

    SDL_RWops* file = SDL_RWFromFile(saveFilePath,"rb");
    Uint8 temp=0;
    if(file)
    {
        //Initial size check (all KSF files need to at least store the entire header)
        if(SDL_RWsize(file) < SAVE_HEADER_SIZE)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"loadGame: Save file is not valid! (File size lower than header size)");
            closeLoadedSaveFile(file);
            return -1;
        }
        //Verify magic number "KSF"
        char magicNum[4] = {'\0'}; //Loaded KSF Magic number
        SDL_RWread(file,magicNum,sizeof(char),3);
        if(strcmp(magicNum,saveMagicNum)!=0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"loadGame: Save file is not valid! ('%s' != 'KSF')",magicNum);
            closeLoadedSaveFile(file);
            return -1;
        }
        //Parse Header
        int gridWidth = SDL_ReadU8(file); // Loaded grid width
        int gridHeight = SDL_ReadU8(file); // Loaded grid height
        //Final size check
        if(SDL_RWsize(file) < ((2*gridWidth*gridHeight)+SAVE_HEADER_SIZE))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"loadGame: Save file is not valid! (File size not big enough to fit all data)");
            closeLoadedSaveFile(file);
            return -1;
        }
        //Parse the rest of header if last check succeeded
        logicData->gridWidth = gridWidth;
        logicData->gridHeight = gridHeight;
        temp = SDL_ReadU8(file); logicData->totalPlayerCount = SDL_min(temp,4);
        temp = SDL_ReadU8(file); logicData->curPlayerCount = SDL_min(temp,4);
        Uint8 aiDifficulty = SDL_ReadU8(file); aiDifficulty = SDL_clamp(aiDifficulty,1,3);
        temp = SDL_ReadU8(file); logicData->curPlayer = SDL_min(temp,4) - 1;
        for(int i=0; i<4; i++)
            loadPlayerStatus(i,SDL_ReadU8(file));
        for(int i=0; i<4; i++)
            loadAIType(i,SDL_ReadU8(file),aiDifficulty);
        int loadedTime = (int)SDL_ReadU8(file) + ((int)SDL_ReadU8(file)*60) + ((int)SDL_ReadU8(file)*3600);
        
        //Parse tile data
        for(int x=0; x<gridWidth; x++)
        {
            for(int y=0; y<gridHeight; y++)
            {
                int player = SDL_ReadU8(file) - 1;
                Uint8 atomCount = SDL_ReadU8(file);
                gamelogic_setAtoms(x,y,player,atomCount);
                logicData->critGrid[x][y] = 4;
                if(x == 0 || x == gridWidth-1)
                    logicData->critGrid[x][y]--;
                if(y == 0 || y == gridHeight-1)
                    logicData->critGrid[x][y]--;
            }
        }
        ai_Init();
        closeLoadedSaveFile(file);
        return loadedTime;
    }
    return -1;
}

/// @brief Convert internal player status to a KSF Player status value
/// @param playerNum Player number (0-3)
/// @return KSF player status value to write as a byte
static Uint8 savePlayerStatus(int playerNum)
{
    switch(logicData->playerStatus[playerNum])
    {
        case PST_PLAYING:
            return 2;
        case PST_NOTSTARTED:
            return 1;
        case PST_LOST:
            return 0;
        case PST_NOTPRESENT:
            return 255;
        default:
            game_errorMsg("savePlayerStatus: Invalid player %d status (%d)",playerNum,logicData->playerStatus[playerNum]);
            break;
    }
    return 255;
}

/// @brief Convert internal AI + difficulty values to a KSF AI Type value
/// @param playerNum Player number (0-3)
/// @return KSF AI Type value to write as a byte
static Uint8 savePlayerAI(int playerNum)
{
    if(aiPlayer[playerNum])
        return aiDifficulty[playerNum] + 1;
    else
        return 0;
}

bool saveGame(void)
{
    // Player numbers saved directly to KSF have to be increased by 1 due to the format being designed for original KleleAtoms, which was made in Lua, a language with 1-indexed arrays.
    // (Specifically the current player and tile player numbers)

    if(!logicData)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"saveGame: Tried to save game without the game being initialized!");
        return false;
    }

    SDL_RWops* file = SDL_RWFromFile(saveFilePath,"wb");
    if(file)
    {
        // Get the current time
        Uint64 ttime = ktimer_getTimeSeconds(gameTimer);

        // Save the KSF header data
        SDL_RWwrite(file,saveMagicNum,sizeof(char),strlen(saveMagicNum));
        SDL_WriteU8(file, logicData->gridWidth & 0xFF);
        SDL_WriteU8(file, logicData->gridHeight & 0xFF);
        SDL_WriteU8(file, logicData->totalPlayerCount & 0xFF);
        SDL_WriteU8(file, logicData->curPlayerCount & 0xFF);
        SDL_WriteU8(file, 2); //AIdifficulty is an unused value, but still present in the save format and has to be in range 1-3
        SDL_WriteU8(file, (logicData->curPlayer + 1) & 0xFF);
        for(int i=0; i<4; i++)
            SDL_WriteU8(file,savePlayerStatus(i));
        for(int i=0; i<4; i++)
            SDL_WriteU8(file,savePlayerAI(i));
        SDL_WriteU8(file,ttime % 60);
        SDL_WriteU8(file,(ttime/60) % 60);
        SDL_WriteU8(file,(ttime/3600) & 0xFF);
        // Save the KSF tile data
        for(int x=0; x<logicData->gridWidth; x++)
        {
            for(int y=0; y<logicData->gridHeight; y++)
            {
                struct KATile* curTile = &logicData->tiles[x][y];
                SDL_WriteU8(file, (curTile->playerNum + 1) & 0xFF);
                SDL_WriteU8(file, (curTile->atomCount) & 0xFF);
            }
        }
        SDL_RWclose(file);
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"saveGame: Couldn't save the game! Reason: %s",SDL_GetError());
        game_printMsg("Couldn't save the game!",3);
        return false;
    }
    return true;
}
