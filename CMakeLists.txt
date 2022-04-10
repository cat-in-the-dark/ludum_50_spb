cmake_minimum_required(VERSION 3.0)
project(atlantis C CXX)

set(CMAKE_CXX_STANDARD 17)

# Setting parameters for raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # or games

if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-multichar")
endif()

if (EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
endif ()

add_subdirectory(libs/raylib)

add_subdirectory(libs/raygui/projects/CMake)

add_executable(${PROJECT_NAME}
    src/collisions.c
    src/game_screen.c
    src/game_screen_3d.c
    src/game_over_screen.c
    src/main.c
    )
target_link_libraries(${PROJECT_NAME} PRIVATE raylib raygui)

target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/") # Set the asset path macro to the absolute path on the dev machine

add_executable(perlin
    src/perlin.c
    )

target_link_libraries(perlin PRIVATE raylib raygui)

add_executable(${PROJECT_NAME}_experiment
    src/test_liquid.c
    )

target_link_libraries(${PROJECT_NAME}_experiment PRIVATE raylib raygui)
