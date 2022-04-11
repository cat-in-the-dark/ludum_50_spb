#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#include "collisions.h"
#include "const.h"
#include "game_screen_3d.h"

#define MAP_W           16
#define MAP_L           16
#define MAP_H           8
#define MAP_CELLS_X     10
#define MAP_CELLS_Y     10

#define BOX_SIZE        1.0f

#define WATER_W         (int)(MAP_W/BOX_SIZE)
#define WATER_L         (int)(MAP_L/BOX_SIZE)
#define WATER_H         (int)(MAP_H/BOX_SIZE + 5)

Camera camera;
Texture2D texture;
Mesh mesh;
Model model;
Vector3 mapPosition;
Ray mouseRay;
RayCollision modelCollision;

TriangleCollisionInfo info;
Vector3 boxPos;

int waterUpdateCounter;

typedef enum {
    EMPTY=0,
    FILLED,
    OCCUPIED
} CellState;

CellState water[WATER_W+2][WATER_H+2][WATER_L+2];
float mass[WATER_W+2][WATER_H+2][WATER_L+2];
float new_mass[WATER_W+2][WATER_H+2][WATER_L+2];

//Water properties
float MaxMass = 1.0f; //The normal, un-pressurized mass of a full water cell
float MaxCompress = 0.02f; //How much excess water a cell can store, compared to the cell above it
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

void TranslateModel(Model* model, Vector3 pos) {
    // Matrix, 4x4 components, column major, OpenGL style, right handed
    // typedef struct Matrix {
    //     float m0, m4, m8, m12;  // Matrix first row (4 components)
    //     float m1, m5, m9, m13;  // Matrix second row (4 components)
    //     float m2, m6, m10, m14; // Matrix third row (4 components)
    //     float m3, m7, m11, m15; // Matrix fourth row (4 components)
    // } Matrix;

    // x = m12; y = m13; z = m14

    model->transform.m12 = pos.x;
    model->transform.m13 = pos.y;
    model->transform.m14 = pos.z;
}

TriangleCollisionInfo CheckWaterBox(int i, int j, int k) {
    Vector3 boxHalf = {BOX_SIZE/2, BOX_SIZE/2, BOX_SIZE/2};
    Vector3 boxPos = Vector3Add((Vector3){i*BOX_SIZE, j*BOX_SIZE, k*BOX_SIZE}, mapPosition);
    BoundingBox testBox = {Vector3Subtract(boxPos, boxHalf), Vector3Add(boxPos, boxHalf)};
    return CheckCollisionBoxMesh(testBox, mesh, model.transform);
}

void InitWater() {
    for (int i = 0; i < WATER_W+2; i++) {
        for (int j = 0; j < WATER_H+2; j++) {
            for (int k = 0; k < WATER_L+2; k++) {
                TriangleCollisionInfo info = CheckWaterBox(i, j, k);
                if (info.hit) {
                    water[i][j][k] = OCCUPIED;
                }
            }
        }
    }

    for (int i = 0; i < WATER_W+2; i++) {
        for (int j = 0; j < WATER_H+2; j++) {
            for (int k = 0; k < WATER_L+2; k++) {
                if (i == 0 || k == 0 || i == WATER_W + 1 || k == WATER_L + 1) {
                    water[i][j][k] = OCCUPIED;
                }
            }
        }
    }

    // memset(water, 0, sizeof(water));
    for (int i = 1; i < WATER_W; i++) {
        for (int j = 1; j < WATER_L; j++) {
            water[i][0][j] = OCCUPIED;
            water[i][WATER_H-1][j] = FILLED;
            water[i][WATER_H-2][j] = FILLED;
            mass[i][WATER_H-1][j] = 1.0;
            mass[i][WATER_H-2][j] = 1.0;
            new_mass[i][WATER_H-1][j] = 1.0;
            new_mass[i][WATER_H-2][j] = 1.0;
            // water[i][1][j] = OCCUPIED;
        }
    }
    
    water[WATER_W-1][WATER_H-1][WATER_L-1] = FILLED;
    mass[WATER_W-1][WATER_H-1][WATER_L-1] = 1.0;
    new_mass[WATER_W-1][WATER_H-1][WATER_L-1] = 1.0;
}

void game_init_3d() {
    printf("%s called\n", __FUNCTION__);

    waterUpdateCounter = 0;

    // Define our custom camera to look into our 3d world
    // camera = (Camera){ { 18.0f, 18.0f, 18.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    camera = (Camera){ { 18.0f, 18.0f, 18.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    // Image image = LoadImage("../assets/heightmap.png");             // Load heightmap image (RAM)
    Image image = GenImageCellular(MAP_CELLS_X, MAP_CELLS_Y, 3);
    ImageColorInvert(&image);
    texture = LoadTextureFromImage(image);                // Convert image to texture (VRAM)

    mesh = GenMeshHeightmap(image, (Vector3){ MAP_W, MAP_H, MAP_L });    // Generate heightmap mesh (RAM and VRAM)
    // mesh = GenMeshCube(10, 10, 10);

    model = LoadModelFromMesh(mesh);                          // Load model from generated mesh

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;         // Set map diffuse texture
    mapPosition = (Vector3){ -MAP_W/2.0f, 0.0f, -MAP_L/2.0f };                   // Define model position
    TranslateModel(&model, mapPosition);

    InitWater();

    boxPos = (Vector3) {0.0f, 0.0f, 0.0f};

    printf("%d %d\n", model.meshes[0].vertexCount, model.meshes[0].triangleCount);

    for (size_t i = 0; i < mesh.vertexCount * 3; i += 3) {
        printf("%f ", mesh.vertices[i]);
        printf("%f\n", mesh.vertices[i+2]);
    }
    printf("\n");

    UnloadImage(image);                     // Unload heightmap image from RAM, already uploaded to VRAM

    SetCameraMode(camera, CAMERA_ORBITAL);  // Set an orbital camera mode
    // SetCameraMode(camera, CAMERA_FREE);  // Set an orbital camera mode
}

void UpdateWater() {
    float flow = 0;
    float remaining_mass;

    for (int x = 0; x < WATER_W; x++) {
        for (int y = 0; y < WATER_H; y++) {
            for (int z = 0; z < WATER_L; z++) {
                if (water[x][y][z] == OCCUPIED) {
                    continue;
                }

                flow = 0;
                remaining_mass = mass[x][y][z];
                if (remaining_mass <= 0) {
                    continue;
                }

                // Below
                if (water[x][y-1][z] != OCCUPIED) {
                    flow = get_stable_state_b(remaining_mass + mass[x][y-1][z]) - mass[x][y-1][z];
                    if (flow > MinFlow) {
                        flow *= 0.5;
                    }
                    flow = Clamp(flow, 0, fmin(MaxSpeed, remaining_mass));

                    new_mass[x][y][z] -= flow;
                    new_mass[x][y-1][z] += flow;

                    remaining_mass -= flow;
                }

                if (remaining_mass <= 0) {
                    continue;
                }

                // Left
                if (water[x-1][y][z] != OCCUPIED) {
                    // Equalize the amount of water in this block and it's neighbour
                    flow = (mass[x][y][z] - mass[x-1][y][z]) / 4;
                    if (flow > MinFlow) {
                        flow *= 0.5;
                    }
                    flow = Clamp(flow, 0, remaining_mass);

                    // printf("[%d;%d;%d]: %.2f -> [LEFT]: %.2f: %f\n", x, y, z, new_mass[x][y][z], new_mass[x-1][y][z], flow);
                    new_mass[x][y][z] -= flow;
                    new_mass[x-1][y][z] += flow;
                    remaining_mass -= flow;
                }

                if (remaining_mass <= 0) {
                    continue;
                }
                
                // Right
                if (water[x+1][y][z] != OCCUPIED) {
                    flow = (mass[x][y][z] - mass[x+1][y][z]) / 4;
                    if (flow > MinFlow) {
                        flow *= 0.5;
                    }
                    flow = Clamp(flow, 0, remaining_mass);

                    new_mass[x][y][z] -= flow;
                    new_mass[x+1][y][z] += flow;
                    remaining_mass -= flow;
                }

                if (remaining_mass <= 0) {
                    continue;
                }
                
                // Front
                if (water[x][y][z-1] != OCCUPIED) {
                    flow = (mass[x][y][z] - mass[x][y][z-1]) / 4;
                    if (flow > MinFlow) {
                        flow *= 0.5;
                    }
                    flow = Clamp(flow, 0, remaining_mass);

                    new_mass[x][y][z] -= flow;
                    new_mass[x][y][z-1] += flow;
                    remaining_mass -= flow;
                }

                
                if (remaining_mass <= 0) {
                    continue;
                }
                
                // Back
                if (water[x][y][z+1] != OCCUPIED) {
                    flow = (mass[x][y][z] - mass[x][y][z+1]) / 4;
                    if (flow > MinFlow) {
                        flow *= 0.5;
                    }
                    flow = Clamp(flow, 0, remaining_mass);

                    new_mass[x][y][z] -= flow;
                    new_mass[x][y][z+1] += flow;
                    remaining_mass -= flow;
                }

                if (remaining_mass <= 0) {
                    continue;
                }

                // Up. Only compressed water flows upwards.

                if (mass[x][y+1][z] != OCCUPIED) {
                    flow = remaining_mass - get_stable_state_b(remaining_mass + mass[x][y+1][z]);
                    if (flow > MinFlow) {
                        flow *= 0.5;
                    }

                    flow = Clamp(flow, 0, fmin(MaxSpeed, remaining_mass));

                    if (flow > 0) {
                        printf("[%d;%d;%d](%.2f) -> [%d;%d;%d](%.2f): %.2f\n", x, y, z, new_mass[x][y][z], x, y+1, z, new_mass[x][y+1][z], flow);
                    }
                    new_mass[x][y][z] -= flow;
                    new_mass[x][y+1][z] += flow;
                    remaining_mass -= flow;
                }
            }
        }
    }

    memcpy(&mass, &new_mass, sizeof(mass));

    for (int x = 1; x < WATER_W; x++) {
        for (int y = 1; y < WATER_H; y++) {
            for (int z = 1; z < WATER_L; z++) {
                if (water[x][y][z] == OCCUPIED) {
                    continue;
                }

                if (mass[x][y][z] > MinMass) {
                    water[x][y][z] = FILLED;
                } else {
                    water[x][y][z] = EMPTY;
                }
            }
        }
    }
}

screen_t game_update_3d() {
    UpdateCamera(&camera);              // Update camera

    waterUpdateCounter++;
    if (waterUpdateCounter % 5 == 0) {
        UpdateWater();    
    }

    mouseRay = GetMouseRay(GetMousePosition(), camera);
    modelCollision = GetRayCollisionMesh(mouseRay, mesh, model.transform);

    return game_screen_3d;
}

void game_draw_3d() {
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

        Vector3 position = { 0.0f, 0.0f, 0.0f };
        DrawModel(model, position, 1.0f, RED);
        // DrawModelWires(model, position, 1.0f, RED);

        DrawGrid(20, 1.0f);

        if (modelCollision.hit) {
            DrawSphere(modelCollision.point, 1, RED);
        }

        // DrawCube(mapPosition, 10, sinf(waterUpdateCounter / 100.0f) * 10, 10, BLUE);


        for (int i = 0; i < WATER_W+2; i++) {
            for (int k = 0; k < WATER_L+2; k++) {
                for (int j = 0; j < WATER_H+2; j++) {
                    Vector3 cubePos = Vector3Add((Vector3){i*BOX_SIZE, j*BOX_SIZE, k*BOX_SIZE}, mapPosition);
                    if (water[i][j][k] == FILLED && mass[i][j][k] >= MinDraw) {
                        float column_mass = 0.0f;
                        int column_height = 0;
                        for (int j1 = j - 1; j1 > 0; j1--) {
                            // calculate total water mass below current block;
                            if (water[i][j1][k] == FILLED) {
                                column_mass += mass[i][j1][k];
                                column_height++;
                            } else if (water[i][j1][k] == OCCUPIED) {
                                break;
                            }
                        }

                        // place box on top of the box below
                        float dy = Clamp(column_height * BOX_SIZE - column_mass * BOX_SIZE, 0, column_height * BOX_SIZE);
                        
                        // align vertically to the botton
                        dy += BOX_SIZE - (BOX_SIZE * mass[i][j][k]) / 2.0f;
                        cubePos = Vector3Subtract(cubePos, (Vector3){0, dy, 0});
                        Vector3 blueHSV = ColorToHSV(BLUE);
                        float colorValue = Remap((float)j / WATER_H, 0.0f, 1.0f, 0.3f, 0.6f);
                        float hue = Remap((float)i / WATER_W, 0.0f, 1.0f, blueHSV.x - 20, blueHSV.x + 20);
                        float sat = Remap((float)k / (WATER_L+2), 0, 1, 0.5, 1);
                        Color color = ColorFromHSV(hue, sat, colorValue);
                        // color.a = (unsigned char)(mass[i][j][k] * 255);
                        DrawCube(cubePos, BOX_SIZE, BOX_SIZE * mass[i][j][k], BOX_SIZE, color);
                        // DrawCylinder(cubePos, BOX_SIZE, BOX_SIZE, BOX_SIZE * mass[i][j][k], 4, color);
                    // } else if (water[i][j][k] == OCCUPIED) {
                    //     DrawCubeWires(cubePos, BOX_SIZE, BOX_SIZE, BOX_SIZE, RED);
                    // } else {
                    //     DrawCubeWires(cubePos, BOX_SIZE, BOX_SIZE, BOX_SIZE, BLACK);
                    }
                }
            }
        }

        if (info.hit) {
            // printf("HIT %f %f %f; %f %f %f; %f %f %f!\n", info.triangle.p1.x, info.triangle.p1.y, info.triangle.p1.z, 
            //     info.triangle.p2.x, info.triangle.p2.y, info.triangle.p2.z, 
            //     info.triangle.p3.x, info.triangle.p3.y, info.triangle.p3.z);
            DrawTriangle3D(info.triangle.p1, info.triangle.p2, info.triangle.p3, RED);
        }

    EndMode3D();

    DrawTexture(texture, SCREEN_WIDTH - texture.width - 20, 20, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - texture.width - 20, 20, texture.width, texture.height, GREEN);

    // if (!modelCollision.hit) {
    //     DrawText("No collision", 10, 42, 32, RED);
    // }

    DrawFPS(10, 10);
}

void game_close_3d() {
    printf("%s called\n", __FUNCTION__);
}

screen_t game_screen_3d = {
    .name = 'GM3D',
    .init = game_init_3d,
    .update = game_update_3d,
    .draw = game_draw_3d,
    .close = game_close_3d
};
