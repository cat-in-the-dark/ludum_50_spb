#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#include "collisions.h"
#include "const.h"
#include "game_screen.h"

#define MAP_W           16
#define MAP_H           16
#define MAP_Z           8
#define MAP_CELLS_X     3
#define MAP_CELLS_Y     3

#define BOX_SIZE        1.0f

Camera camera;
Texture2D texture;
Mesh mesh;
Model model;
Vector3 mapPosition;
Ray mouseRay;
RayCollision modelCollision;

TriangleCollisionInfo info;
Vector3 boxPos;

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

void game_init() {
    printf("%s called\n", __FUNCTION__);
    // Define our custom camera to look into our 3d world
    // camera = (Camera){ { 18.0f, 18.0f, 18.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    camera = (Camera){ { 18.0f, 18.0f, 18.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    // Image image = LoadImage("../assets/heightmap.png");             // Load heightmap image (RAM)
    Image image = GenImageCellular(MAP_CELLS_X, MAP_CELLS_Y, 2);
    ImageColorInvert(&image);
    texture = LoadTextureFromImage(image);                // Convert image to texture (VRAM)

    mesh = GenMeshHeightmap(image, (Vector3){ MAP_H, MAP_Z, MAP_W });    // Generate heightmap mesh (RAM and VRAM)
    // mesh = GenMeshCube(10, 10, 10);

    model = LoadModelFromMesh(mesh);                          // Load model from generated mesh

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;         // Set map diffuse texture
    mapPosition = (Vector3){ -MAP_W/2.0f, 0.0f, -MAP_H/2.0f };                   // Define model position
    TranslateModel(&model, mapPosition);

    boxPos = (Vector3) {0.0f, 0.0f, 0.0f};

    printf("%d %d\n", model.meshes[0].vertexCount, model.meshes[0].triangleCount);

    for (size_t i = 0; i < mesh.vertexCount * 3; i += 3) {
        printf("%f ", mesh.vertices[i]);
        printf("%f\n", mesh.vertices[i+2]);
    }
    printf("\n");

    UnloadImage(image);                     // Unload heightmap image from RAM, already uploaded to VRAM

    // SetCameraMode(camera, CAMERA_ORBITAL);  // Set an orbital camera mode
    SetCameraMode(camera, CAMERA_FREE);  // Set an orbital camera mode
}

screen_t game_update() {
    UpdateCamera(&camera);              // Update camera
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

    return game_screen;
}

void game_draw() {
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

        Vector3 position = { 0.0f, 0.0f, 0.0f };
        // DrawModel(model, mapPosition, 1.0f, RED);
        DrawModelWires(model, position, 1.0f, RED);

        DrawGrid(20, 1.0f);

        if (modelCollision.hit) {
            DrawSphere(modelCollision.point, 1, RED);
        }

        DrawCube(boxPos, BOX_SIZE, BOX_SIZE, BOX_SIZE, BLUE);

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
