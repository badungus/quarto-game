# Command-line solver: proves positions as win, draw or loss. Built on Odin's
# core, so it shares Position, the win checks, canonical keys and the TT.
add_executable(quarto_solve main.cpp)

target_link_libraries(quarto_solve PRIVATE odin_core)
