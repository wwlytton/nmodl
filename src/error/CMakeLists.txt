# =============================================================================
# Error sources
# =============================================================================
set(ERROR_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/error_decl.hpp)

# =============================================================================
# Error library and executable
# =============================================================================
add_library(error STATIC ${ERROR_SOURCE_FILES})

add_dependencies(error lexer util visitor)
