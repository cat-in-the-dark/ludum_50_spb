#include <stdio.h>

#include "raylib.h"

#include "game_screen.h"

void game_init() {
    printf("%s called\n", __FUNCTION__);
}

screen_t game_update() {
    printf("%s called\n", __FUNCTION__);
    return game_screen;
}

void game_draw() {
    printf("%s called\n", __FUNCTION__);
    ClearBackground(BLUE);
}

void game_close() {
    printf("%s called\n", __FUNCTION__);
}

screen_t game_screen = {
    .name = 'GAME',
    .init = game_init,
    .update = game_update,
    .draw = game_draw,
    .close = game_close
};
