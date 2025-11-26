cmake_minimum_required(VERSION 3.11)

set(CMAKE_C_STANDARD 17)

project(KleleAtoms-PSP)

if(PSP)
set(WAVPLAYER src/utils/wavplayerpsp.c)   
else()
set(WAVPLAYER src/utils/wavplayermix.c)   
endif()

add_executable(KleleAtoms-PSP 
    src/main.c
    src/game/game.c
    src/game/state.c
    src/game/save.c
    src/game/fade.c
    src/game/assetman.c
    src/utils/timer.c
    ${WAVPLAYER}
    src/utils/rendertext.c
    src/utils/pakread.c
    src/states/menu/menustate.c
    src/states/menu/menuatoms.c
    src/states/menu/menuui.c
    src/states/game/gamestate.c
    src/states/game/gamelogic.c
    src/states/game/gamedraw.c
    src/states/game/gameai.c
    src/states/game/gametutorial.c
)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)

if(PSP)
set(ADDITIONAL_LIBS "pspdebug" "pspdisplay")
set(ADDITIONAL_INCLUDES "")
else()
pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)
set(ADDITIONAL_LIBS "m" ${SDL2_MIXER_LIBRARIES})
set(ADDITIONAL_INCLUDES ${SDL2_MIXER_INCLUDE_DIRS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${ADDITIONAL_INCLUDES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${ADDITIONAL_LIBS}
)

if(PSP)
    create_pbp_file(
        TARGET ${PROJECT_NAME}
        ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ICON0.PNG
        BACKGROUND_PATH ${CMAKE_CURRENT_SOURCE_DIR}/KABG.PNG
        PREVIEW_PATH NULL
        TITLE "KleleAtoms PSP v1.1 Dev"
        VERSION 01.10
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/kleleatoms
    )
endif()
