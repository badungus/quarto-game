cmake_minimum_required(VERSION 3.20)

project(quarto-game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

add_executable(quarto
    src/main.cpp
    src/engine/ordering.cpp
    src/engine/position.cpp
    src/engine/search.cpp
    src/engine/tt.cpp
    src/engine/wincheck.cpp
    src/game/game_state.cpp
    src/ui/app.cpp
    src/ui/screens/game_screen.cpp
    src/ui/widgets/button.cpp
)

target_include_directories(quarto PRIVATE include)

target_link_libraries(quarto PRIVATE raylib)
