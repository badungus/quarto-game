# Search core as a library: the engine executable links it, and tests can too.
add_library(quarto_engine_core STATIC
    engine.cpp
    ordering.cpp
    position.cpp
    search.cpp
    tt.cpp
    uqi.cpp
    wincheck.cpp
)

target_include_directories(quarto_engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Standalone engine process, spoken to over UQI on stdin/stdout.
add_executable(quarto-engine main.cpp)

target_link_libraries(quarto-engine PRIVATE quarto_engine_core)
