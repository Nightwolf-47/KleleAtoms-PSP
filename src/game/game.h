#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "../utils/pspwav.h"

#ifndef STR
#define STR(s) #s
#endif

#ifndef XSTR
#define XSTR(s) STR(s)
#endif

extern SDL_Renderer* gameRenderer;

extern PSPWav* sfxExplode; //Explode sound
extern PSPWav* sfxPut; //Atom put sound
extern PSPWav* sfxClick; //Menu button click sound

//PSP Screen width, has to be 480
#define SCREEN_WIDTH 480
//PSP Screen height, has to be 272
#define SCREEN_HEIGHT 272

//Highest grid width
#define MAX_GRIDWIDTH 13
//Lowest supported grid width setting
#define MIN_GRIDWIDTH 5

//Highest grid height
#define MAX_GRIDHEIGHT 8
//Lowest supported grid height setting
#define MIN_GRIDHEIGHT 4

struct KASettings {
    int gridWidth;
    int gridHeight;
    int player1Type;
    int player2Type;
    int player3Type;
    int player4Type;
};

extern struct KASettings gameSettings;

// If true, the game state is launched in tutorial mode, otherwise it's in normal mode
extern bool launchedTutorial;

/// @brief Initialize the game and all subsystems
/// @return true on success, false on failure
bool game_init(void);

// Initialize the game loop and run it until the game stops
void game_loop(void);

/// @brief Close the game with an error message
/// @param format Printf-like string followed by additional arguments
void game_errorMsg(const char* format, ...);

/// @brief Print info message string for a given time
/// @param str Message string
/// @param time Time to show the message for
void game_printMsg(const char* str, float time);

// Close all the game subsystems before quitting the game
void game_quit(void);
