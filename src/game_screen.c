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
    int powerConsumption;
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

int totalFood;
int totalConcrete;

int powerCapacity;
int powerUsage;

int totalPopulation;
int populationCapacity;

int gameTicks;

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
    gameTicks = 0;
    totalFood = 0;
    totalConcrete = 200;
    totalPopulation = 0;
    powerUsage = 0;
    powerCapacity = 0;

    memset(buildings, 0, sizeof(buildings));
    initBalance();
    initBuildings();

    printf("%s called\n", __FUNCTION__);
}

void updatePower() {
    float power = 0;
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type == POWER_PLANT) {
            power += buildings[i].powerProduction;
        }
    }

    powerCapacity = power;
}

void updateResources() {
    float foodIncrement = 0;
    float concreteIncrement = 0;
    float powerLeft = powerCapacity;

    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (powerLeft <= 0) {
            break;
        }
        
        if (buildings[i].type != BUILDING_INVALID) {
            powerLeft -= buildings[i].powerConsumption;
        }

        if (buildings[i].resource == FOOD) {
            foodIncrement += buildings[i].productionRate;
        } else if (buildings[i].resource == CONCRETE) {
            concreteIncrement += buildings[i].productionRate;
        }
    }
    
    totalFood += foodIncrement;
    totalConcrete += concreteIncrement;
    powerUsage = powerCapacity - powerLeft;
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

    int delta = (totalFood - totalPopulation) / 2;
    printf("Food: %d; Pop: %d; delta: %d\n", totalFood, totalPopulation, delta);

    totalPopulation += delta;
    if (totalPopulation < 0) {
        totalPopulation = 0;
    } else if (totalPopulation > housingCapacity) {
        totalPopulation = housingCapacity;
    }

    totalFood -= totalPopulation;
    if (totalFood < 0) {
        totalFood = 0;
    }

    printf("After: %d / %d\n", totalFood, totalPopulation);
}

screen_t game_update() {
    gameTicks++;
    if (gameTicks % 30 == 0) {
        updatePower();
        updateResources();
        updatePopulation();
    }
    
    return game_screen;
}

void game_draw() {
    ClearBackground(RAYWHITE);
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].type != BUILDING_INVALID) {
            buildings[i].draw(&buildings[i]);
        }
    }
    
    DrawText(TextFormat("Food: %d; Concrete: %d; power: %d/%d;\npopulation: %d",
        totalFood, totalConcrete, powerUsage, powerCapacity, totalPopulation),
        10, 42, 32, BLACK);

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
