# Search core as a library, linked by the engine executable.
add_library(odin_core STATIC
    canonical.cpp
    engine.cpp
    position.cpp
    search.cpp
    tt.cpp
    uqi.cpp
    wincheck.cpp
)

target_include_directories(odin_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Standalone engine process, spoken to over UQI on stdin/stdout.
add_executable(odin main.cpp)

target_link_libraries(odin PRIVATE odin_core)
