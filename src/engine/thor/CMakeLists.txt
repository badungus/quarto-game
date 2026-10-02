# Search core as a library, linked by the engine executable.
add_library(thor_core STATIC
    engine.cpp
    position.cpp
    search.cpp
    uqi.cpp
    wincheck.cpp
)

target_include_directories(thor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Standalone engine process, spoken to over UQI on stdin/stdout.
add_executable(thor main.cpp)

target_link_libraries(thor PRIVATE thor_core)
