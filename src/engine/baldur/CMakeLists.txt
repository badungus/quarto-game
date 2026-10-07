# Search core as a library, linked by the engine executable.
add_library(baldur_core STATIC
    engine.cpp
    position.cpp
    search.cpp
    uqi.cpp
    wincheck.cpp
)

target_include_directories(baldur_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Standalone engine process, spoken to over UQI on stdin/stdout.
add_executable(baldur main.cpp)

target_link_libraries(baldur PRIVATE baldur_core)
