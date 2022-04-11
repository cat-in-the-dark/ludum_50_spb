#include <stdio.h>

#include "game_screen.h"
#include "game_screen_3d.h"
#include "game_over_screen.h"

#include "raylib.h"

void game_over_init() {
    printf("%s called!\n", __FUNCTION__);
}

screen_t game_over_update() {
    if (IsKeyPressed(KEY_ENTER)) {
        return game_screen_3d;
    } 

    return game_over_screen;
}

void game_over_draw() {
    ClearBackground(RAYWHITE);
    DrawText(" ", 10, 10, 32, BLACK);
}

void game_over_close() {
    printf("%s called!\n", __FUNCTION__);
}

screen_t game_over_screen = {
    .name = 'GMVR',
    .init = game_over_init,
    .update = game_over_update,
    .draw = game_over_draw,
    .close = game_over_close
};
