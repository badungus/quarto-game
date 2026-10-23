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

add_search_engine(odin_no_canonical
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE ROOT_BEST_FIRST
    THREAT_ORDERING)

add_search_engine(odin_no_threat_ordering
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL ROOT_BEST_FIRST)

add_search_engine(odin_no_safe_gives
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE TT TT_MOVE CANONICAL ROOT_BEST_FIRST
    THREAT_ORDERING)

add_search_engine(odin_no_tt_move
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT CANONICAL ROOT_BEST_FIRST
    THREAT_ORDERING)

add_search_engine(odin_no_root_best_first
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL
    THREAT_ORDERING)

add_search_engine(odin_no_narrow_root_window
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL ROOT_BEST_FIRST
    THREAT_ORDERING)

# Incremental ladder: baldur plus one feature per step, in an order that meets
# every dependency. Step 4 is thor and step 11 would be odin, so neither is
# built again.
add_search_engine(ladder_01_piece_count_score
    PIECE_COUNT_SCORE)

add_search_engine(ladder_02_win_check_at_entry
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY)

add_search_engine(ladder_03_threat_eval
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL)

add_search_engine(ladder_05_completes_table
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE)

add_search_engine(ladder_06_safe_gives
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES)

add_search_engine(ladder_07_tt
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT)

add_search_engine(ladder_08_tt_move
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE)

add_search_engine(ladder_09_canonical
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL)

add_search_engine(ladder_10_root_best_first
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL ROOT_BEST_FIRST)

