# KleleAtoms-PSP
PSP port of [KłełeAtoms](https://github.com/Nightwolf-47/KleleAtoms), a game where you have to take atoms from other players by blowing up yours.  
  
<img width="400" height="225" alt="kapsp-menu" src="https://github.com/user-attachments/assets/20a663bb-af25-4caf-91b3-32c5f2e2b4db" />
<img width="400" height="225" alt="kapsp-game" src="https://github.com/user-attachments/assets/152ed69d-4557-4146-a1e9-30ce85e04a28" />
<img width="400" height="225" alt="kapsp-pause" src="https://github.com/user-attachments/assets/262b434a-a50e-4df8-94d0-2d9ca522d5ef" />
<img width="400" height="225" alt="kapsp-menu2" src="https://github.com/user-attachments/assets/db45a83c-1793-4491-bc5a-c72976cf58bb" />


## How to play

The game starts with an empty grid. Every player can place 1 atom per turn on tiles without enemy atoms.  
When a tile has too many* atoms, it becomes critical and explodes. Each surrounding tile gains an atom and all existing atoms on those tiles go to the player who started the explosion.  
If an explosion causes other atoms to become critical, they will explode as well, causing a chain reaction.  
If a player loses all their atoms, they lose. The last standing player wins the game.  
  
\*corner - 2 atoms, side - 3 atoms, otherwise 4 atoms (or more)  

## Options
- Grid width - Set in-game grid width (5-13)  
- Grid height - Set in-game grid height (4-8)  
- Player 1-4 type - Set player type (None, Human, AI 1, AI 2, AI 3) - only 2 players can be None at the same time.  

## Third-party libraries used
- [SDL2](https://github.com/libsdl-org/SDL/tree/SDL2) (License: [Zlib](https://github.com/libsdl-org/SDL/blob/SDL2/LICENSE.txt))
- [SDL2_image](https://github.com/libsdl-org/SDL_image/tree/SDL2) (License: [Zlib](https://github.com/libsdl-org/SDL_image/blob/SDL2/LICENSE.txt))
- [stb_truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h) + [stb_rect_pack](https://github.com/nothings/stb/blob/master/stb_rect_pack.h) (License: [Unlicense/MIT](https://github.com/nothings/stb/blob/master/LICENSE))  

## Credits
- **PSPDEV Organization** - PSPSDK used for the development of this game.
- **DrPetter** - SFXR, a tool used to make sounds for KleleAtoms.
- **greffmaster** - Beta testing + PSVita testing.

## Build instructions
1. [Install the PSPSDK](https://pspdev.github.io/installation.html). The required libraries should be bundled with the SDK.
    - If your SDK doesn't have the libraries preinstalled, you can type  
    `psp-pacman -S sdl2 sdl2-image stb`  
    in the terminal with which you installed PSPSDK.
2. Install the Python programming language. It is required for the resource archive packing scripts.
3. Use one of the build_\*.sh scripts in a Linux/WSL terminal with which you installed PSPSDK:
    - **build_debug.sh** - Creates a debug build of the game.
    - **build_release.sh** - Creates a release build of the game.
    - **build_release_signed.sh** - Creates a signed release build of the game. This build works on PSPs without CFW, but it's usually bigger than the regular release build.
    - **build_clean.sh** - Cleans up the compiled files and everything in `build/kleleatoms` directory, forcing the compiler to recompile the entire game the next time you use a build script. Does not actually build the game.
4. The compiled build should be present in `build/kleleatoms` directory. To play it on a PSP, copy the entire `kleleatoms` directory to the PSP/GAME folder.  

## License
This game is licensed under the MIT License, see [LICENSE](https://github.com/Nightwolf-47/KleleAtoms-PSP/blob/main/LICENSE) for details.  
  
The font res/font/DejaVuSans.ttf is licensed under its own license present in the [res/font/LICENSE](https://github.com/Nightwolf-47/KleleAtoms-PSP/blob/main/res/font/LICENSE) file.
