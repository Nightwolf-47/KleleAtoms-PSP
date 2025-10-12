#pragma once
#include <stdbool.h>

// Load settings (without bounds checking)
void loadSettings(void);

// Save settings
void saveSettings(void);

/// @brief Checks if save file is present
/// @return true if save file is present, false otherwise
bool isSavePresent(void);

/// @brief Load the game from KSF (KleleAtoms Save Format)
/// @return New game time in seconds
int loadGame(void);

/// @brief Save the game to KSF (KleleAtoms Save Format) 
/// @return true if saving the game succeeded, false otherwise
bool saveGame(void);
