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

# Pinned to a commit rather than v2.0.4: the tag still asks for CMake 3.1, which
# CMake 4 refuses to configure.
FetchContent_Declare(
    tiny-process-library
    GIT_REPOSITORY https://gitlab.com/eidheim/tiny-process-library.git
    GIT_TAG 8bbb5a211c5c9df8ee69301da9d22fb977b27dc1
)

FetchContent_MakeAvailable(raylib tiny-process-library)

# Every executable lands in the build root: the GUI launches quarto-engine from
# its own directory, and assets are loaded relative to it (../assets).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src/engine)

add_executable(quarto
    src/main.cpp
    src/game/game_state.cpp
    src/ui/app.cpp
    src/ui/input.cpp
    src/ui/screens/game_screen.cpp
    src/ui/screens/screen.cpp
    src/ui/widgets/button.cpp
    src/ui/theme/assets.cpp
    src/match/match_controller.cpp
    src/players/human_player.cpp
    src/players/engine_player.cpp
)

target_include_directories(quarto PRIVATE include)

target_link_libraries(quarto PRIVATE raylib tiny-process-library::tiny-process-library)

# Building the GUI also builds the engine it launches.
add_dependencies(quarto quarto-engine)
