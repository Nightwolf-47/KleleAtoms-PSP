cmake_minimum_required(VERSION 3.11)

set(CMAKE_C_STANDARD 17)

project(KleleAtoms-PSP)

add_executable(KleleAtoms-PSP 
    src/main.c
    src/game/game.c
    src/game/state.c
    src/game/save.c
    src/game/fade.c
    src/game/assetman.c
    src/utils/timer.c
    src/utils/wavplayer.c
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

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

if(PSP)
set(ADDITIONAL_LIBS "pspdebug" "pspdisplay")
else()
set(ADDITIONAL_LIBS "m")
endif()

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
        TITLE "KleleAtoms PSP v1.0.1 Dev"
        VERSION 01.01
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/kleleatoms
    )
endif()
