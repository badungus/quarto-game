# One search engine, built once per preset. A preset is a set of features
# (see features.hpp), each turned on by a compile definition; every preset
# needs its own compile of the sources, so each gets its own core library.
#
#   add_search_engine(<name> [FEATURE ...])
#
# builds <name>_core and the executable <name>, spoken to over UQI on
# stdin/stdout. FEATURE is a name from features.hpp, e.g. PIECE_COUNT_SCORE.
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

# Presets. Named merged_* while the old engine directories still build the
# real names, so both can be compared.
add_search_engine(merged_baldur)
add_search_engine(merged_thor
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW)
add_search_engine(merged_odin
    PIECE_COUNT_SCORE WIN_CHECK_AT_ENTRY THREAT_EVAL NARROW_ROOT_WINDOW
    COMPLETES_TABLE SAFE_GIVES TT TT_MOVE CANONICAL ROOT_BEST_FIRST
    THREAT_ORDERING)
