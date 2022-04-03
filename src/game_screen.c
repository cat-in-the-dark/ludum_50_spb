#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#include "collisions.h"
#include "const.h"
#include "game_screen.h"

#define MAX_BUILDINGS 255

#define BALANCE_POWER_PRODUCTION    100
#define BALANCE_PEOPLE_CAPACITY     100

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
    float powerConsumption;
    Rectangle body;
    void (*draw)(struct Building_*);
    ResourceType resource;
    float productionRate;
    union {
        char padding[4];
        int peopleCapacity;
        int powerProduction;        
    };
} Building;

Balance balance[LAST];

Building buildings[MAX_BUILDINGS];

float totalFood;
float totalConcrete;

float powerCapacity;
float powerUsage;

float population;
float populationCapacity;

// Init

// Update

void flashError() {
    // TODO: show error on screen somehow
}

void drawBuilding(Building* building) {
    Rectangle body = building->body;
    DrawRectangleLines(body.x, body.y, body.width, body.height, BLACK);
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
    addHouse(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT));
    addHouse(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT));
    addFarm(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT));
    addPowerPlant(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT));
    addConcreteFactory(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT));
}

void initBalance() {
    balance[HOUSE] = (Balance) {
        .price = 40,
        .powerConsumption = 30,
        .resource = RES_INVALID,
        .productionRate = 0.0f,
        .width = 50,
        .height = 50
    };

    balance[FARM] = (Balance) {
        .price = 20,
        .powerConsumption = 10,
        .resource = FOOD,
        .productionRate = 2.0f,
        .width = 50,
        .height = 25
    };

    balance[CONCRETE_FACTORY] = (Balance) {
        .price = 100,
        .powerConsumption = 50,
        .resource = CONCRETE,
        .productionRate = 10.0f,
        .width = 75,
        .height = 25
    };

    balance[POWER_PLANT] = (Balance) {
        .price = 50,
        .powerConsumption = 0,
        .resource = RES_INVALID,
        .productionRate = 0.0f,
        .width = 25,
        .height = 50
    };
}

void game_init() {
    memset(buildings, 0, sizeof(buildings));
    initBalance();
    initBuildings();
    totalFood = 0;
    totalConcrete = 200;
    population = 0;
    printf("%s called\n", __FUNCTION__);
}

screen_t game_update() {
    return game_screen;
}

void game_draw() {
    ClearBackground(RAYWHITE);
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type != BUILDING_INVALID) {
            buildings[i].draw(&buildings[i]);
        }
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
