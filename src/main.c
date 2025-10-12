#include "game/game.h"

int main(int argc, char** argv)
{
    if(!game_init())
        return 1;

    game_loop();

    game_quit();

    return 0;
}
