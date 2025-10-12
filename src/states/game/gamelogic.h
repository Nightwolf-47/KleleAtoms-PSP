#pragma once
#include <stdbool.h>
#include <stdint.h>

// Size of the atom stack / explosion count limit
#define ATOMSTACKSIZE 5000
// Limit of visible atoms per tile
#define MAX_VISIBLE_ATOMS 8
#define NOPLAYER -1

// Max supported grid width
#define MAX_GRID_WIDTH 13
// Max supported grid height
#define MAX_GRID_HEIGHT 8

enum PlayerStatus
{
    PST_NOTPRESENT,     //The player doesn't exist (None option in settings)
    PST_LOST,           //The player lost the game
    PST_NOTSTARTED,     //The player didn't place any atoms yet
    PST_PLAYING         //The player is playing
};

// Simple x,y int struct
typedef struct Vec2 {
    int x;
    int y;
} Vec2;

// Simple x,y float struct
typedef struct Vec2f {
    float x;
    float y;
} Vec2f;

struct KAAtom {
    float curx;     //Current atom X position (relative to tile it's on)
    float cury;     //Current atom Y position (relative to tile it's on)
    float endx;     //Destination atom X position (relative to tile it's on)
    float endy;     //Destination atom Y position (relative to tile it's on)
};

struct KATile {
    int playerNum;                              //Tile player number (equal to NOPLAYER if no player has atoms on that tile)
    struct KAAtom atoms[MAX_VISIBLE_ATOMS];     //Array of atom positions and destination positions (limited to MAX_VISIBLE_ATOMS)
    int atomCount;                              //Amount of atoms in a tile
    float explodeTime;                          //Time left until explosion disappears
};

struct KAWillExplode {
    int x;          //X tile position of the tile that will explode
    int y;          //Y tile position of the tile that will explode
    bool explode;   //Will a tile explode? If false, other values are irrelevant
};

struct GameLogicData {
    struct KATile tiles[MAX_GRID_WIDTH][MAX_GRID_HEIGHT];   //Struct of tile data indexed as [x][y]
    int critGrid[MAX_GRID_WIDTH][MAX_GRID_HEIGHT];          //Struct of critical atom amounts per tile
    int gridWidth;                                          //Current grid width in tiles
    int gridHeight;                                         //Current grid height in tiles
    bool animPlaying;                                       //TRUE if animation is playing
    int curPlayer;                                          //Currently playing player number
    enum PlayerStatus playerStatus[4];                      //Player statuses (PlayerStatus enums)
    int playerAtoms[4];                                     //Player atom count
    int curPlayerCount;                                     //Current player count (not counting players who lost)
    int totalPlayerCount;                                   //Total player count (counting players who lost)
    int explosionCount;                                     //Current explosion count (restarts every new turn)
    int playerWon;                                          //Player number that won the game (NOPLAYER if the game has not ended)
    struct KAWillExplode willExplode;                       //Struct holding tile that will explode soon (if willExplode.explode == true)
    Vec2 atomStack[ATOMSTACKSIZE];                          //Atom stack, used to store previous explosion positions for chain reactions
    int atomStackPos;                                       //Current atom stack position
};

// The main game data
extern struct GameLogicData* logicData;

/// @brief Set atoms on a tile without animations
/// @param x Tile X position
/// @param y Tile Y position
/// @param player New tile player, sets to logic->curPlayer if equal to NOPLAYER
/// @param atomCount New atom count on the tile
void gamelogic_setAtoms(int x, int y, int player, uint32_t atomCount);

/// @brief Sets the new player types (should only be used in init and tutorial)
/// @param playerTypes Pointer to player type array (corresponding to player type settings)
void gamelogic_setPlayers(int (*playerTypes)[4]);

/// @brief Initializes the game
/// @param gridWidth Grid width
/// @param gridHeight Grid height
/// @param playerTypes Pointer to player type array (corresponding to player type settings)
void gamelogic_init(int gridWidth, int gridHeight, int (*playerTypes)[4]);

/// @brief Runs a game tick
/// @param dt Delta time from update state callback
void gamelogic_tick(float dt);

/// @brief Clicks a tile
/// @param x Tile X position
/// @param y Tile Y position
/// @param isAIMove If true, will work even when current player is AI
void gamelogic_clickedTile(int x, int y, bool isAIMove);

// Deallocates logicData memory allocated by gamelogic_init
void gamelogic_stop(void);
