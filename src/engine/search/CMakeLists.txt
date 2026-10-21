function(add_search_engine name)
    add_library(${name}_core STATIC
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/canonical.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/engine.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ordering.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/position.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/search.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tt.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/uqi.cpp
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/wincheck.cpp
    )
    target_include_directories(${name}_core PUBLIC
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
    foreach(feature IN LISTS ARGN)
        target_compile_definitions(${name}_core PUBLIC Q_${feature}=1)
    endforeach()

    add_executable(${name} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/main.cpp)
    target_link_libraries(${name} PRIVATE ${name}_core)
endfunction()

# Presets. Until tag pre-merge each was its own copy of the source in
# src/engine/<name>; these give output identical to those.
add_search_engine(baldur)
add_search_engine(thor
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW)
add_search_engine(odin
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL ROOT_BEST_FIRST
    THREAT_ORDERING)

# Ablation variants: odin with features switched off. Features that need a
# switched-off one (see features.hpp) go with it.
add_search_engine(odin_no_tt
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES ROOT_BEST_FIRST THREAT_ORDERING)
