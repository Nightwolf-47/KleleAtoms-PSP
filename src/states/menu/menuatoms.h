#pragma once
#include <SDL2/SDL.h>

// Max amount of background main menu atom textures on the screen
#define MAX_MENU_ATOMS 20

// Data for a background atom texture in main menu
struct MenuAtom {
    float x;                // X atom position
    float y;                // Y atom position
    float xspeed;           // horizontal movement speed
    float yspeed;           // vertical movement speed (should be > 0)
    int atomType;           // Amount of atoms in the texture (1-3)
    SDL_Color atomColor;  // Atom color
};

extern struct MenuAtom menuAtoms[MAX_MENU_ATOMS]; // Array containing menu background atom data
extern int menuAtomCount; // Current amount of atoms in the menu background

/// @brief Add a menu atom texture if menuAtomCount < MAX_MENU_ATOMS
/// @param color The color of the new atom
void menuatoms_spawnAtom(SDL_Color color);

/// @brief Move the menu atoms according to their x and y speed variables
/// @param dt DeltaTime from update state callback
void menuatoms_moveAtoms(float dt);
