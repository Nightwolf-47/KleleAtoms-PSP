#include "menuatoms.h"
#include "../../game/game.h"
#include <SDL2/SDL.h>

struct MenuAtom menuAtoms[MAX_MENU_ATOMS];
int menuAtomCount;

// Returns a random float roughly between min and max
static float randomFloat(float min, float max)
{
    max -= min;
    return ((float)rand() / (float)(RAND_MAX/max)) + min;
}

// Removes an atom that went out of bounds
static void removeAtom(int index)
{
    for(int i=index+1; i<menuAtomCount; i++)
    {
        menuAtoms[i-1] = menuAtoms[i];
    }
    menuAtomCount--;
}

void menuatoms_spawnAtom(SDL_Color color)
{
    if(menuAtomCount >= MAX_MENU_ATOMS)
        return;

    struct MenuAtom* newAtom = &menuAtoms[menuAtomCount++];
    newAtom->y = -48;
    newAtom->x = rand() % 420 + 30;
    newAtom->xspeed = randomFloat(-150,150);
    newAtom->yspeed = randomFloat(75,150);
    newAtom->atomType = rand() % 3;
    newAtom->atomColor = color;
}

void menuatoms_moveAtoms(float dt)
{
    int menuAtomIndex = 0;
    while(menuAtomIndex < menuAtomCount)
    {
        struct MenuAtom* curAtom = &menuAtoms[menuAtomIndex];
        curAtom->x += curAtom->xspeed*dt;
        curAtom->y += curAtom->yspeed*dt;
        if(curAtom->x <= -48 || curAtom->x >= 528 || curAtom->y >= 320)
            removeAtom(menuAtomIndex);
        else
            menuAtomIndex++;
    }
}
