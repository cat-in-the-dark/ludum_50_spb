#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#include "const.h"
#include "game_screen.h"

Camera camera;
Texture2D texture;
Mesh mesh;
Model model;
Vector3 mapPosition;
Ray mouseRay;
RayCollision modelCollision;

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
    camera = (Camera){ { 18.0f, 18.0f, 18.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    // Image image = LoadImage("../assets/heightmap.png");             // Load heightmap image (RAM)
    Image image = GenImageCellular(128, 128, 32);
    ImageColorInvert(&image);
    texture = LoadTextureFromImage(image);                // Convert image to texture (VRAM)

    mesh = GenMeshHeightmap(image, (Vector3){ 16, 8, 16 });    // Generate heightmap mesh (RAM and VRAM)
    // mesh = GenMeshCube(10, 10, 10);
    model = LoadModelFromMesh(mesh);                          // Load model from generated mesh

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;         // Set map diffuse texture
    mapPosition = (Vector3){ -8.0f, 0.0f, -8.0f };                   // Define model position
    TranslateModel(&model, mapPosition);
    UnloadImage(image);                     // Unload heightmap image from RAM, already uploaded to VRAM

    SetCameraMode(camera, CAMERA_ORBITAL);  // Set an orbital camera mode
}

screen_t game_update() {
    printf("%s called\n", __FUNCTION__);
    UpdateCamera(&camera);              // Update camera
    mouseRay = GetMouseRay(GetMousePosition(), camera);
    modelCollision = GetRayCollisionMesh(mouseRay, mesh, model.transform);
    return game_screen;
}

void game_draw() {
    printf("%s called\n", __FUNCTION__);
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

        Vector3 position = { 0.0f, 0.0f, 0.0f };
        // DrawModel(model, mapPosition, 1.0f, RED);
        DrawModel(model, position, 1.0f, RED);

        DrawGrid(20, 1.0f);

        if (modelCollision.hit) {
            DrawSphere(modelCollision.point, 1, RED);
        }

        DrawRay(mouseRay, RED);

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
