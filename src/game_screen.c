#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"
#include "raygui.h"

#include "collisions.h"
#include "const.h"
#include "game_screen.h"
#include "game_over_screen.h"

#define MAX_BUILDINGS 255
#define MAX_PLATFORMS 255

#define BALANCE_POWER_PRODUCTION    100
#define BALANCE_PEOPLE_CAPACITY     100
#define BALANCE_POPULATION_INCREMENT    10
#define BALANCE_WATER_LEVEL_SPEED   0.05f
#define BALANCE_WATER_START_POS     SCREEN_HEIGHT
#define BALANCE_MAP_WIDTH           SCREEN_WIDTH
#define BALANCE_PLATFORM_ANGLE      60
#define BALANCE_PLATFORM_WIDTH      100
#define BALANCE_PLATFORM_LEG_LENGTH 75

typedef struct {
    union {
        struct {
            Vector2 start;
            Vector2 end;
        };
        Vector2 coords[2];
    };
} Line;

typedef struct {
    union {
        struct {
            float x;
            float y;
        };
        Vector2 pos;
    };
    float width;
    float height;
} MyRectangle;

typedef struct {
    MyRectangle rect;
    float angle;
} RotRectangle;

typedef struct {
    union {
        struct {
            RotRectangle left;
            RotRectangle right;
            RotRectangle top;
        };
        RotRectangle components[3];
    };
} Platform;

typedef struct {
    bool hitLeft;
    bool hitRight;
    Vector2 pointLeft;
    Vector2 pointRight;
} PlatformCollision;

typedef struct {
    bool active;
    Platform platform;
} ActivePlatform;

typedef enum {
    BUILDING_INVALID=0,
    HOUSE,
    POWER_PLANT,
    CONCRETE_FACTORY,
    FARM,
    LAST
} BuildingType;

// Power is not a resource!
typedef enum {
    RES_INVALID=0,
    FOOD,
    CONCRETE
} ResourceType;

typedef struct {
    ResourceType type;
} Resource;

typedef struct {
    int price;
    int powerConsumption;
    ResourceType resource;
    float productionRate;
    int width;
    int height;
} Balance;

typedef struct Building_ {
    BuildingType type;
    int powerConsumption;
    bool powered;
    Rectangle body;
    void (*draw)(struct Building_*);
    ResourceType resource;
    int productionRate;
    union {
        char padding[4];
        int peopleCapacity;
        int powerProduction;        
    };
} Building;

Balance balance[LAST];

Building buildings[MAX_BUILDINGS];

Line g_ground[1];

ActivePlatform g_activePlatform;
Platform* g_platforms;
int g_platformsCount;

int g_totalFood;
int g_totalConcrete;

int g_powerCapacity;
int g_powerUsage;
int g_powerRequired;

int g_totalPopulation;
int populationCapacity;

int g_gameTicks;

float g_waterLevel;

int sign(int x) {
    return (x > 0) - (x < 0);
}

void flashError() {
    // TODO: show error on screen somehow
}

Line GetRotRectangleMiddleLine(RotRectangle rotRectangle) {
    Vector2 startP = {rotRectangle.rect.x, rotRectangle.rect.y};
    Vector2 lineStart = {rotRectangle.rect.x, rotRectangle.rect.y + rotRectangle.rect.height / 2};
    Vector2 lineEnd = {rotRectangle.rect.x + rotRectangle.rect.width, rotRectangle.rect.y};
    Vector2 deltaX = Vector2Subtract(lineStart, startP);
    deltaX = Vector2Rotate(deltaX, rotRectangle.angle * DEG2RAD);
    lineStart = Vector2Add(startP, deltaX);

    Vector2 deltaY = Vector2Subtract(lineEnd, startP);
    deltaY = Vector2Rotate(deltaY, rotRectangle.angle * DEG2RAD);
    lineEnd = Vector2Add(Vector2Add(startP, deltaX), deltaY);
    return (Line){lineStart, lineEnd};
}

Platform GeneratePlatform(int x, int y, int topWidth, int topHeight, int legLength, int legThickness, int legAngle) {
    printf("Generate: %d %d %d %d %d %d %d\n", x, y, topWidth, topHeight, legLength, legThickness, legAngle);
    // x; y = coordinates of top platform
    Platform platform;
    platform.top.rect = (MyRectangle){x, y, topWidth, topHeight};
    platform.top.angle = 0;

    platform.left.rect = (MyRectangle){x, y, legLength, legThickness};
    platform.left.angle = 180 - legAngle;

    platform.right.rect = (MyRectangle){x, y, legLength, legThickness};
    platform.right.angle = legAngle;

    Line topMiddleLine = GetRotRectangleMiddleLine(platform.top);
    Line leftMiddleLine = GetRotRectangleMiddleLine(platform.left);
    Line rightMiddleLine = GetRotRectangleMiddleLine(platform.right);

    Vector2 leftDist = Vector2Subtract(topMiddleLine.start, leftMiddleLine.start);
    platform.left.rect.pos = Vector2Add(leftDist, platform.left.rect.pos);

    Vector2 rightDist = Vector2Subtract(topMiddleLine.end, rightMiddleLine.start);
    platform.right.rect.pos = Vector2Add(rightDist, platform.right.rect.pos);
    return platform;
}

void AddPlatform(Platform platform) {
    if (g_platforms == NULL) {
        g_platforms = (Platform*)MemAlloc(sizeof(Platform));
    } else {
        g_platforms = (Platform*)MemRealloc(g_platforms, sizeof(Platform) * (g_platformsCount  + 1));
    }

    g_platforms[g_platformsCount] = platform;
    g_platformsCount++;
}

void drawBuilding(Building* building) {
    Rectangle body = building->body;
    Color color = BLACK;
    if (!building->powered) {
        color = RED;
    }

    DrawRectangleLines(body.x, body.y, body.width, body.height, color);
}

void addBuilding(Building* building) {
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type == BUILDING_INVALID) {
            memcpy(&buildings[i], building, sizeof(Building));
            return;
        }
    }

    printf("Unable to add building!!!\n");
    flashError();
}

Building initBuilding(BuildingType type, int x, int y) {
    return (Building) {
        .type = type,
        .powerConsumption = balance[type].powerConsumption,
        .body = (Rectangle){x, y, balance[type].width, balance[type].height},
        .resource = balance[type].resource,
        .productionRate = balance[type].productionRate
    };
}

void addPowerPlant(int x, int y) {
    Building powerPlant = initBuilding(POWER_PLANT, x, y);
    powerPlant.draw = drawBuilding;
    powerPlant.powerProduction = BALANCE_POWER_PRODUCTION;

    addBuilding(&powerPlant);
}

void addFarm(int x, int y) {
    Building farm = initBuilding(FARM, x, y);
    farm.draw = drawBuilding;

    addBuilding(&farm);
}

void addHouse(int x, int y) {
    Building house = initBuilding(HOUSE, x, y);
    house.draw = drawBuilding;
    house.peopleCapacity = BALANCE_PEOPLE_CAPACITY;

    addBuilding(&house);
}

void addConcreteFactory(int x, int y) {
    Building concreteFactory = initBuilding(CONCRETE_FACTORY, x, y);
    concreteFactory.draw = drawBuilding;

    addBuilding(&concreteFactory);
}

void initBuildings() {
    // TODO: stick to the ground properly
    addHouse(GetRandomValue(0, SCREEN_WIDTH), 420 - balance[HOUSE].height);
    addHouse(GetRandomValue(0, SCREEN_WIDTH), 420 - balance[HOUSE].height);
    addPowerPlant(GetRandomValue(0, SCREEN_WIDTH), 420 - balance[POWER_PLANT].height);
    addFarm(GetRandomValue(0, SCREEN_WIDTH), 420 - balance[FARM].height);
    addConcreteFactory(GetRandomValue(0, SCREEN_WIDTH), 420 - balance[CONCRETE_FACTORY].height);
}

void initGround() {
    // TODO: Fancier generation
    g_ground[0] = (Line) {
        (Vector2) {0, 420},
        (Vector2) {BALANCE_MAP_WIDTH, 420}
    };
}

void initBalance() {
    balance[HOUSE] = (Balance) {
        .price = 40,
        .powerConsumption = 20,
        .resource = RES_INVALID,
        .productionRate = 0,
        .width = 50,
        .height = 50
    };

    balance[FARM] = (Balance) {
        .price = 20,
        .powerConsumption = 10,
        .resource = FOOD,
        .productionRate = 50,
        .width = 50,
        .height = 25
    };

    balance[CONCRETE_FACTORY] = (Balance) {
        .price = 100,
        .powerConsumption = 50,
        .resource = CONCRETE,
        .productionRate = 10,
        .width = 75,
        .height = 25
    };

    balance[POWER_PLANT] = (Balance) {
        .price = 50,
        .powerConsumption = 0,
        .resource = RES_INVALID,
        .productionRate = 0,
        .width = 25,
        .height = 50
    };
}

void game_init() {
    g_gameTicks = 0;
    g_totalFood = 1000;
    g_totalConcrete = 200;
    g_totalPopulation = 100;
    g_powerUsage = 0;
    g_powerCapacity = 0;
    g_powerRequired = 0;
    g_waterLevel = 0;

    memset(buildings, 0, sizeof(buildings));
    initBalance();
    initGround();
    initBuildings();
    g_platformsCount = 0;
    g_platforms = NULL;

    AddPlatform(GeneratePlatform(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), BALANCE_PLATFORM_WIDTH, 10, BALANCE_PLATFORM_LEG_LENGTH, 10, BALANCE_PLATFORM_ANGLE));
    AddPlatform(GeneratePlatform(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 100, 10, 75, 10, BALANCE_PLATFORM_ANGLE));
    AddPlatform(GeneratePlatform(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT), 100, 10, 75, 10, BALANCE_PLATFORM_ANGLE));

    printf("%s called\n", __FUNCTION__);
}

void updatePower() {
    float power = 0;
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type == POWER_PLANT) {
            power += buildings[i].powerProduction;
        }
    }

    g_powerCapacity = power;
}

void updateResources() {
    int foodIncrement = 0;
    int concreteIncrement = 0;
    int powerLeft = g_powerCapacity;
    g_powerRequired = 0;

    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type != BUILDING_INVALID) {
            g_powerRequired += buildings[i].powerConsumption;
            if (powerLeft >= buildings[i].powerConsumption) {
                powerLeft -= buildings[i].powerConsumption;
                buildings[i].powered = true;

                if (buildings[i].resource == FOOD) {
                    foodIncrement += buildings[i].productionRate;
                } else if (buildings[i].resource == CONCRETE) {
                    concreteIncrement += buildings[i].productionRate;
                }
            } else {
                buildings[i].powered = false;
            }
        }
    }

    g_totalFood += foodIncrement;
    g_totalConcrete += concreteIncrement;
    g_powerUsage = g_powerCapacity - powerLeft;
}

void updatePopulation() {
    // calculate how many people we can feed
    int housingCapacity = 0;
    // calculate maximum people capacity
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type == HOUSE) {
            housingCapacity += buildings[i].peopleCapacity;
        }
    }

    int delta = g_totalFood - g_totalPopulation;
    if (abs(delta) > BALANCE_POPULATION_INCREMENT) {
        delta = sign(delta) * BALANCE_POPULATION_INCREMENT;
    }

    g_totalPopulation += delta;
    if (g_totalPopulation < 0) {
        g_totalPopulation = 0;
    } else if (g_totalPopulation > housingCapacity) {
        g_totalPopulation = housingCapacity;
    }

    g_totalFood -= g_totalPopulation;
    if (g_totalFood < 0) {
        g_totalFood = 0;
    }
}

void UpdateWaterLevel() {
    g_waterLevel += BALANCE_WATER_LEVEL_SPEED;

    Rectangle waterRect = {0, BALANCE_WATER_START_POS - g_waterLevel, BALANCE_MAP_WIDTH, g_waterLevel};
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type != BUILDING_INVALID) {
            if (CheckCollisionRecs(buildings[i].body, waterRect)) {
                buildings[i].type = BUILDING_INVALID;
            }
        }
    }
}

PlatformCollision CheckCollisionPlatformGround(Platform platform) {
    PlatformCollision result = {0};
    Vector2 collisionPoint = {0};

    Line leftMl = GetRotRectangleMiddleLine(platform.left);
    Line rightMl = GetRotRectangleMiddleLine(platform.right);

    for (int i = 0; i < ARR_SIZE(g_ground); i++) {
        if (result.hitLeft && result.hitRight) {
            return result;
        }

        if (!result.hitLeft && CheckCollisionLines(g_ground[i].start, g_ground[i].end, leftMl.start, leftMl.end, &collisionPoint)) {
            result.hitLeft = true;
            result.pointLeft = collisionPoint;
        }

        if (!result.hitRight && CheckCollisionLines(g_ground[i].start, g_ground[i].end, rightMl.start, rightMl.end, &collisionPoint)) {
            result.hitRight = true;
            result.pointRight = collisionPoint;
        }
    }

    return result;
}

PlatformCollision CheckCollisionPlatforms(Platform pl) {
    PlatformCollision result = {0};
    Vector2 collisionPoint = {0};

    Line leftMl = GetRotRectangleMiddleLine(pl.left);
    Line rightMl = GetRotRectangleMiddleLine(pl.right);

    for (int i = 0; i < g_platformsCount; i++) {
        for (int j = 0; j < ARR_SIZE(pl.components); j++) {
            if (result.hitLeft && result.hitRight) {
                return result;
            }

            Line targetMl = GetRotRectangleMiddleLine(g_platforms[i].components[j]);

            if (!result.hitLeft && CheckCollisionLines(leftMl.start, leftMl.end, targetMl.start, targetMl.end, &collisionPoint)) {
                result.hitLeft = true;
                result.pointLeft = collisionPoint;
            }

            if (!result.hitRight && CheckCollisionLines(rightMl.start, rightMl.end, targetMl.start, targetMl.end, &collisionPoint)) {
                result.hitRight = true;
                result.pointRight = collisionPoint;
            }
        }
    }

    return result;
}

void UpdateControls() {
    Vector2 mouse = GetMousePosition();

    if (IsKeyPressed(KEY_P)) {
        if (!g_activePlatform.active) {
            g_activePlatform.platform = GeneratePlatform((int)mouse.x, (int)mouse.y, BALANCE_PLATFORM_WIDTH, 10, BALANCE_PLATFORM_LEG_LENGTH, 10, BALANCE_PLATFORM_ANGLE);
            g_activePlatform.active = true;
        }
    }

    if (g_activePlatform.active) {
        // TODO: convert coordinates from screen to world, when camera added
        Vector2 dPos = Vector2Subtract(mouse, g_activePlatform.platform.top.rect.pos);
        g_activePlatform.platform.top.rect.pos = mouse;
        g_activePlatform.platform.left.rect.pos = Vector2Add(g_activePlatform.platform.left.rect.pos, dPos);
        g_activePlatform.platform.right.rect.pos = Vector2Add(g_activePlatform.platform.right.rect.pos, dPos);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlatformCollision againstPlatforms = CheckCollisionPlatforms(g_activePlatform.platform);
            PlatformCollision againstGround = CheckCollisionPlatformGround(g_activePlatform.platform);

            if ((againstGround.hitLeft || againstPlatforms.hitLeft) && (againstGround.hitRight || againstPlatforms.hitRight)) {
                AddPlatform(g_activePlatform.platform);
                g_activePlatform.active = false;
            }
        }
    }
}

screen_t game_update() {
    g_gameTicks++;
    if (g_gameTicks % 30 == 0) {
        updatePower();
        updateResources();
        updatePopulation();
    }
    
    UpdateControls();
    UpdateWaterLevel();

    if (g_totalPopulation <= 0) {
        return game_over_screen;
    } else {
        return game_screen;
    }
}

void drawBuildings() {
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type != BUILDING_INVALID) {
            buildings[i].draw(&buildings[i]);
        }
    }
}

void drawGround() {
    for (int i = 0; i < ARR_SIZE(g_ground); i++) {
        DrawLineV(g_ground[0].start, g_ground[0].end, BLACK);
    }    
}

void drawWater() {
    DrawRectangle(0, BALANCE_WATER_START_POS - g_waterLevel, BALANCE_MAP_WIDTH, g_waterLevel, BLUE);
}

void drawHud() {
    DrawText(TextFormat("Food: %d; Concrete: %d; power: %d/%d;\npopulation: %d",
        g_totalFood, g_totalConcrete, g_powerRequired, g_powerCapacity, g_totalPopulation),
        10, 42, 20, BLACK);

    DrawFPS(10, 10);
}

void DrawPlatform(Platform platform) {
    DrawRectangleLines(platform.top.rect.x, platform.top.rect.y, platform.top.rect.width, platform.top.rect.height, BLACK);
    Line middleLeft = GetRotRectangleMiddleLine(platform.left);
    Line middleRight = GetRotRectangleMiddleLine(platform.right);
    DrawLineV(middleLeft.start, middleLeft.end, BLACK);
    DrawLineV(middleRight.start, middleRight.end, BLACK);
}

void DrawPlatforms() {
    for (int i = 0; i < g_platformsCount; i++) {
        DrawPlatform(g_platforms[i]);
    }

    if (g_activePlatform.active) {
        DrawPlatform(g_activePlatform.platform);
    }
}

void game_draw() {
    ClearBackground(RAYWHITE);
    DrawPlatforms();
    drawBuildings();
    drawGround();
    drawWater();
    drawHud();
}

void game_close() {
    printf("%s called\n", __FUNCTION__);
    if (g_platforms != NULL) {
        MemFree(g_platforms);
    }
}

screen_t game_screen = {
    .name = 'GAME',
    .init = game_init,
    .update = game_update,
    .draw = game_draw,
    .close = game_close
};
