#include <math.h>
#include <stdio.h>


#include "raylib.h"
#include "raymath.h"

#include "const.h"

#define W   SCREEN_WIDTH - 40
#define H   SCREEN_HEIGHT - 60
#define CELL_SIZE   32.0f

#define GRID_SIZE 128

typedef struct {
    bool occupied;
    Vector2 coords;
} GradientCell;

GradientCell gradients[GRID_SIZE][GRID_SIZE];

Vector2 randomGradient(int ix, int iy) {
    // assert(ix < GRID_SIZE);
    // assert(iy < GRID_SIZE);

    if (!gradients[ix][iy].occupied) {
        float angle = Remap(GetRandomValue(0, 1000), 0, 1000, 0.0, PI * 2);
        gradients[ix][iy].coords.x = cosf(angle);
        gradients[ix][iy].coords.y = sinf(angle);
        gradients[ix][iy].occupied = true;
    }

    return gradients[ix][iy].coords;
}

// /* Create pseudorandom direction vector
//  */
// Vector2 randomGradient(int ix, int iy) {
//     // No precomputed gradients mean this works for any number of grid coordinates
//     const unsigned w = 8 * sizeof(unsigned);
//     const unsigned s = w / 2; // rotation width
//     unsigned a = ix, b = iy;
//     a *= 3284157443; b ^= a << s | a >> w-s;
//     b *= 1911520717; a ^= b << s | b >> w-s;
//     a *= 2048419325;
//     float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
//     Vector2 v;
//     v.x = cosf(random); v.y = sinf(random);
//     return v;
// }

// Computes the dot product of the distance and gradient vectors.
float dotGridGradient(int ix, int iy, float x, float y) {
    // Get gradient from integer coordinates
    Vector2 gradient = randomGradient(ix, iy);

    // Compute the distance vector
    Vector2 dist = {x - (float)ix, y - (float)iy};

    return Vector2DotProduct(dist, gradient);
}

float interpolate(float a0, float a1, float w) {
    // cubic interpolation
    return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
}

// Compute Perlin noise at coordinates x, y
float perlin(float x, float y) {
    // Determine grid cell coordinates
    int x0 = (int)floor(x);
    int x1 = x0 + 1;
    int y0 = (int)floor(y);
    int y1 = y0 + 1;

    // Determine interpolation weights
    // Could also use higher order polynomial/s-curve here
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = interpolate(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return (value + 1.0) / 2.0;
}

Image GenImagePerlin(int width, int height) {
    Image image = GenImageColor(width, height, RAYWHITE);
    for (int i = 0; i < W; i++)
    {
        for (int j = 0; j < H; j++)
        {
            // float value1 = 
            float noise = (perlin(i / CELL_SIZE, j / CELL_SIZE)
                + 0.5 * perlin(i / CELL_SIZE * 2, j / CELL_SIZE * 2)
                + 0.25 * perlin(i / CELL_SIZE * 4, j / CELL_SIZE * 4))
                    / 1.75;
            // [0.0; 1.0] => [0; 255]
            unsigned char value = (unsigned char)(noise * 255);
            if (i == 0) {
                printf("%f -> %d\n", noise, value);
            }
            Color color = {value, value, value, 255};
            ImageDrawPixel(&image, i, j, color);
        }
    }

    return image;
}

int main(int argc, char const *argv[])
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window title");
    SetTargetFPS(60);

    // Define our custom camera to look into our 3d world
    Camera camera = { { 18.0f, 18.0f, 18.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Image image = GenImagePerlin(128, 128);
    Texture2D texture = LoadTextureFromImage(image);                // Convert image to texture (VRAM)

    Mesh mesh = GenMeshHeightmap(image, (Vector3){ 16, 8, 16 });    // Generate heightmap mesh (RAM and VRAM)
    Model model = LoadModelFromMesh(mesh);                          // Load model from generated mesh

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;         // Set map diffuse texture
    Vector3 mapPosition = { -8.0f, 0.0f, -8.0f };                   // Define model position

    UnloadImage(image);                     // Unload heightmap image from RAM, already uploaded to VRAM

    SetCameraMode(camera, CAMERA_ORBITAL);  // Set an orbital camera mode

    SetTargetFPS(60);                       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);              // Update camera
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                DrawModel(model, mapPosition, 1.0f, RED);

                DrawGrid(20, 1.0f);

            EndMode3D();

            DrawTexture(texture, SCREEN_WIDTH - texture.width - 20, 20, WHITE);
            DrawRectangleLines(SCREEN_WIDTH - texture.width - 20, 20, texture.width, texture.height, GREEN);

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(texture);     // Unload texture
    UnloadModel(model);         // Unload model

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
