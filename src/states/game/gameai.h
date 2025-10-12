#pragma once
#include <stdbool.h>

// Array of whether a player is AI (true) or not (false)
extern bool aiPlayer[4];

// Array of AI player difficulties (1-3, only if player is AI)
extern int aiDifficulty[4];

// Initializes AI values
void ai_Init(void);

// Resets AI delay time
void ai_ResetTime(void);

// Tries to make the AI move, succeeds if the AI delay has passed
void ai_TryMove(void);
