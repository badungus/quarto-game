# Solver core as a library, linked by the engine executable.
add_library(optimal_core STATIC
    canonical.cpp
    engine.cpp
    ordering.cpp
    position.cpp
    solve.cpp
    tt.cpp
    uqi.cpp
    wincheck.cpp
)

target_include_directories(optimal_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Tuned for the CPU it is built on, so std::popcount is one instruction.
# About 1.3 times faster on the full solve.
target_compile_options(optimal_core PUBLIC -march=native)

# Standalone engine process, spoken to over UQI on stdin/stdout; --full
# solves the game and writes the opening book instead.
add_executable(optimal main.cpp)

target_link_libraries(optimal PRIVATE optimal_core)

# The opening book lives next to these sources, so it is versioned with them
# and survives a clean build. Both the engine and --full use this path.
target_compile_definitions(optimal PRIVATE
    OPTIMAL_BOOK_PATH="${CMAKE_CURRENT_SOURCE_DIR}/book.bin")

target_compile_definitions(optimal PRIVATE
    OPTIMAL_SQUARES_BOOK_PATH="${CMAKE_CURRENT_SOURCE_DIR}/square_book.bin")
