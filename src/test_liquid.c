#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#include "const.h"

#define MAP_WIDTH   16
#define MAP_HEIGTH  16

#define CELL_WIDTH  20
#define CELL_HEIGTH 20
#define STARTX      300
#define STARTY      40

typedef enum {
    AIR=0,
    GROUND,
    WATER
} BlockType;

BlockType blocks[MAP_WIDTH+2][MAP_HEIGTH+2];
float mass[MAP_WIDTH+2][MAP_HEIGTH+2];
float new_mass[MAP_WIDTH+2][MAP_HEIGTH+2];

//Water properties
float MaxMass = 1.0f; //The normal, un-pressurized mass of a full water cell
float MaxCompress = 0.1f; //How much excess water a cell can store, compared to the cell above it
float MinMass = 0.0001f;  //Ignore cells that are almost dry
float MinFlow = 0.1f;
float MinDraw = 0.05f;
float MaxSpeed = 4.0f;   //max units of water moved out of one block to another, per timestep

float get_stable_state_b(float total_mass) {
    if (total_mass <= 1) {
        return 1.0f;
    } else if (total_mass < 2 * MaxMass + MaxCompress) {
        // if the top cell contains less than MaxMass units of water, the bottom cell will contain a proportionally smaller excess amount
        return (MaxMass * MaxMass + total_mass * MaxCompress) / (MaxMass + MaxCompress);
    } else {
        return (total_mass + MaxCompress) / 2;
    }
}

void simulate_compression() {
    float flow = 0;
    float remaining_mass;

    for (int x = 1; x < MAP_WIDTH; x++) {
        for (int y = 1; y < MAP_HEIGTH; y++) {
            if (blocks[x][y] == GROUND) {
                continue;
            }

            flow = 0;
            remaining_mass = mass[x][y];
            if (remaining_mass <= 0) {
                continue;
            }

            // Below
            if (blocks[x][y+1] != GROUND) {
                flow = get_stable_state_b(remaining_mass + mass[x][y+1]) - mass[x][y+1];
                if (flow > MinFlow) {
                    flow *= 0.5;
                }
                flow = Clamp(flow, 0, fmin(MaxSpeed, remaining_mass));

                // printf("[%d][%d](%.2f) -> [%d][%d](%.2f): %.2f\n", x, y, new_mass[x][y], x, y+1, new_mass[x][y+1], flow);
                new_mass[x][y] -= flow;
                new_mass[x][y+1] += flow;
                remaining_mass -= flow;
            }

            if (remaining_mass <= 0) {
                continue;
            }
            
            // Left
            if (blocks[x-1][y] != GROUND) {
                // Equalize the amount of water in this block and it's neighbour
                flow = (mass[x][y] - mass[x-1][y]) / 4;
                if (flow > MinFlow) {
                    flow *= 0.5;
                }
                flow = Clamp(flow, 0, remaining_mass);

                // printf("[%d][%d](%.2f) -> [%d][%d](%.2f): %.2f\n", x, y, new_mass[x][y], x-1, y, new_mass[x-1][y], flow);
                new_mass[x][y] -= flow;
                new_mass[x-1][y] += flow;
                remaining_mass -= flow;
            }

            if (remaining_mass <= 0) {
                continue;
            }

            // Right
            if (blocks[x+1][y] != GROUND) {
                // Equalize the amount of water in this block and it's neighbour
                flow = (mass[x][y] - mass[x+1][y]) / 4;
                if (flow > MinFlow) {
                    flow *= 0.5;
                }
                flow = Clamp(flow, 0, remaining_mass);

                // printf("[%d][%d](%.2f) -> [%d][%d](%.2f): %.2f\n", x, y, new_mass[x][y], x+1, y, new_mass[x+1][y], flow);
                new_mass[x][y] -= flow;
                new_mass[x+1][y] += flow;
                remaining_mass -= flow;
            }

            if (remaining_mass <= 0) {
                continue;
            }

            // Up. Only compressed water flows upwards.

            if (blocks[x][y-1] != GROUND) {
                flow = remaining_mass - get_stable_state_b(remaining_mass + mass[x][y-1]);
                if (flow > MinFlow) {
                    flow *= 0.5;
                }

                flow = Clamp(flow, 0, fmin(MaxSpeed, remaining_mass));

                // printf("[%d][%d](%.2f) -> [%d][%d](%.2f): %.2f\n", x, y, new_mass[x][y], x, y-1, new_mass[x][y-1], flow);
                new_mass[x][y] -= flow;
                new_mass[x][y-1] += flow;
                remaining_mass -= flow;
            }
        }
    }

    memcpy(&mass, &new_mass, sizeof(mass));

    for (int x = 1; x < MAP_WIDTH; x++) {
        for (int y = 1; y < MAP_HEIGTH; y++) {
            if (blocks[x][y] == GROUND) {
                continue;
            }

            if (mass[x][y] > MinMass) {
                blocks[x][y] = WATER;
            } else {
                blocks[x][y] = AIR;
            }
        }
    }

    for (int x = 0; x < MAP_WIDTH + 2; x++){
        mass[x][0] = 0;
        mass[x][MAP_HEIGTH+1] = 0;
    }

    for (int y = 1; y < MAP_HEIGTH + 1; y++){
        mass[0][y] = 0;
        mass[MAP_WIDTH+1][y] = 0;
    }
}

void draw_map() {
    for (int x = 0; x < MAP_WIDTH + 2; x++) {
        for (int y = 0; y < MAP_HEIGTH + 2; y++) {
            if (blocks[x][y] == AIR) {
                continue;
            }

            Color color = BLUE;
            if (blocks[x][y] == GROUND) {
                color = BROWN;
            } else {
                color.a = 255 * Clamp(mass[x][y], 0, MaxMass);
            }

            DrawRectangle(STARTX + x * CELL_WIDTH, STARTY + y * CELL_HEIGTH, CELL_WIDTH, CELL_HEIGTH, color);
        }
    }
}

void init_map() {
    for (int x = 0; x < MAP_WIDTH + 2; x++){
        blocks[x][MAP_HEIGTH+1] = GROUND;
        blocks[x][MAP_HEIGTH] = GROUND;
    }

    for (int y = 0; y < MAP_HEIGTH+2; y++) {
        blocks[0][y] = GROUND;
        blocks[MAP_WIDTH+1][y] = GROUND;
        blocks[MAP_WIDTH][y] = GROUND;
    }

    for (int y = MAP_HEIGTH * 0.3; y < MAP_HEIGTH * 0.75; y++) {
        blocks[MAP_WIDTH / 2][y] = GROUND;
        blocks[MAP_WIDTH / 2 + 1][y] = GROUND;
    }

    blocks[2][1] = WATER;
    mass[2][1] = MaxMass;
    new_mass[2][1] = MaxMass;
}

int main(int argc, char const *argv[]) {
    int game_ticks = 0;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window title");

    init_map();

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        game_ticks++;
        // if (game_ticks % 5 == 0) {
        if (1) {
            simulate_compression();

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 pos = GetMousePosition();
                int cx = (pos.x - STARTX) / CELL_WIDTH;
                int cy = (pos.y - STARTY) / CELL_HEIGTH;
                printf("click on [%d][%d]\n", cx, cy);
                if (cx < MAP_WIDTH + 2 && cy < MAP_HEIGTH + 2 && cx > 0 && cy > 0) {
                    BlockType type = blocks[cx][cy];
                    printf("Is %s\n", type == WATER ? "water" : type == AIR ? "air" : "ground");
                    if (type != GROUND) {
                        blocks[cx][cy] = WATER;
                        mass[cx][cy] = MaxMass;
                        new_mass[cx][cy] = MaxMass;
                    }
                } else {
                    printf("Is invalid\n");
                }
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            draw_map();

            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
