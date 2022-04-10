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
#define MAP_CELLS_X     5
#define MAP_CELLS_Y     5

#define BOX_SIZE        1.0f

#define WATER_W         (int)(MAP_W/BOX_SIZE)
#define WATER_L         (int)(MAP_L/BOX_SIZE)
#define WATER_H         (int)(MAP_H/BOX_SIZE)

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

CellState water[WATER_W][WATER_H][WATER_L];

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
    for (int i = 0; i < WATER_W; i++) {
        for (int j = 0; j < WATER_H; j++) {
            for (int k = 0; k < WATER_L; k++) {
                TriangleCollisionInfo info = CheckWaterBox(i, j-1, k);
                if (info.hit) {
                    water[i][j-1][k] = OCCUPIED;
                }
            }
        }
    }
}

void game_init_3d() {
    printf("%s called\n", __FUNCTION__);

    waterUpdateCounter = 0;

    // memset(water, 0, sizeof(water));
    // for (int i = 0; i < WATER_W; i++) {
    //     for (int j = 0; j < WATER_L; j++) {
    //         water[i][WATER_H-1][j] = FILLED;
    //     }
    // }
    
    water[WATER_W-1][WATER_H-1][WATER_L-1] = FILLED;

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
    int fill_count = 0;
    for (int j = 0; j < WATER_H; j++) {
        bool filled = false;
        for (int i = 0; i < WATER_W; i++) {
            for (int k = 0; k < WATER_L; k++) {
                if (water[i][j][k] == FILLED) {
                    // Check the box right below
                    if (j > 0 && water[i][j-1][k] == EMPTY) {
                        water[i][j-1][k] = FILLED;
                        filled = true;
                        fill_count++;
                    } else {
                        // Check neighbouring cells
                        // i-1; k
                        if (i > 0 && water[i-1][j][k] == EMPTY) {
                            // Fill if only cell right below it isn't empty or doesn't exist
                            if (j == 0 || water[i-1][j-1][k] != EMPTY) {
                                water[i-1][j][k] = FILLED;
                                filled = true;
                                fill_count++;
                            }
                        } else if (k > 0 && water[i][j][k-1] == EMPTY) {
                            if (j == 0 || water[i][j-1][k-1] != EMPTY) {
                                water[i][j][k-1] = FILLED;
                                filled = true;
                                fill_count++;
                            }
                        } else if (i < WATER_H - 1 && water[i+1][j][k] == EMPTY) {
                            if (j == 0 || water[i+1][j-1][k] != EMPTY) {
                                water[i+1][j][k] = FILLED;
                                filled = true;
                                fill_count++;
                            }
                        } else if (k < WATER_L -1 && water[i][j][k+1] == EMPTY) {
                            if (j == 0 || water[i][j-1][k+1] != EMPTY) {
                                water[i][j][k+1] = FILLED;
                                filled = true;
                                fill_count++;
                            }
                        }
                    }

                    if (fill_count > 3) {
                        return;
                    }
                }
            }
        }

        if (filled) {
            break;
        }
    }
}

screen_t game_update_3d() {
    UpdateCamera(&camera);              // Update camera

    waterUpdateCounter++;
    if (waterUpdateCounter % 30 == 0) {
        UpdateWater();    
    }

    mouseRay = GetMouseRay(GetMousePosition(), camera);
    modelCollision = GetRayCollisionMesh(mouseRay, mesh, model.transform);

    float speed = .1f;

    if (IsKeyDown(KEY_A)) {
        boxPos.x -= speed;
    }
    if (IsKeyDown(KEY_D)) {
        boxPos.x += speed;
    }
    if (IsKeyDown(KEY_W)) {
        boxPos.y -= speed;
    }
    if (IsKeyDown(KEY_S)) {
        boxPos.y += speed;
    }
    if (IsKeyDown(KEY_Q)) {
        boxPos.z -= speed;
    }
    if (IsKeyDown(KEY_E)) {
        boxPos.z += speed;
    }

    Vector3 boxHalf = {BOX_SIZE/2, BOX_SIZE/2, BOX_SIZE/2};    // boxPos = center position
    BoundingBox testBox = {Vector3Subtract(boxPos, boxHalf), Vector3Add(boxPos, boxHalf)};
    info = CheckCollisionBoxMesh(testBox, mesh, model.transform);

    return game_screen_3d;
}

void game_draw_3d() {
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

        Vector3 position = { 0.0f, 0.0f, 0.0f };
        // DrawModel(model, mapPosition, 1.0f, RED);
        DrawModelWires(model, position, 1.0f, RED);

        DrawGrid(20, 1.0f);

        if (modelCollision.hit) {
            DrawSphere(modelCollision.point, 1, RED);
        }


        for (int i = 0; i < WATER_W; i++) {
            for (int j = 0; j < WATER_H; j++) {
                for (int k = 0; k < WATER_L; k++) {
                    if (water[i][j][k] == FILLED) {
                        Vector3 cubePos = Vector3Add((Vector3){i*BOX_SIZE, j*BOX_SIZE, k*BOX_SIZE}, mapPosition);
                        DrawCube(cubePos, BOX_SIZE, BOX_SIZE, BOX_SIZE, BLUE);
                    // } else if (water[i][j][k] == OCCUPIED) {
                        // Vector3 cubePos = Vector3Add((Vector3){i*BOX_SIZE, j*BOX_SIZE, k*BOX_SIZE}, mapPosition);
                        // DrawCube(cubePos, BOX_SIZE, BOX_SIZE, BOX_SIZE, RED);
                    }
                }
            }
        }

        if (info.hit) {
            printf("HIT %f %f %f; %f %f %f; %f %f %f!\n", info.triangle.p1.x, info.triangle.p1.y, info.triangle.p1.z, 
                info.triangle.p2.x, info.triangle.p2.y, info.triangle.p2.z, 
                info.triangle.p3.x, info.triangle.p3.y, info.triangle.p3.z);
            DrawTriangle3D(info.triangle.p1, info.triangle.p2, info.triangle.p3, RED);
        }

    EndMode3D();

    DrawTexture(texture, SCREEN_WIDTH - texture.width - 20, 20, WHITE);
    DrawRectangleLines(SCREEN_WIDTH - texture.width - 20, 20, texture.width, texture.height, GREEN);

    if (!modelCollision.hit) {
        DrawText("No collision", 10, 42, 32, RED);
    }

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
