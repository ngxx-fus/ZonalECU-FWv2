# ==============================================================================
# Component: Common
# Contains shared definitions: Error codes, Macros, Atomic types, Shared APIs
# ==============================================================================

# 1. Collect all C source files in this component
file(GLOB_RECURSE component_srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

# 2. Collect all directories containing header (.h) files
file(GLOB_RECURSE component_headers
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

set(component_include_dirs "")
foreach(header ${component_headers})
    get_filename_component(header_dir ${header} DIRECTORY)
    list(APPEND component_include_dirs ${header_dir})
endforeach()

# Append parent directory (comp/) so consumers can use: #include "Common/xxx.h"
list(APPEND component_include_dirs "${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}/../..")

if(component_include_dirs)
    list(REMOVE_DUPLICATES component_include_dirs)
    list(SORT component_include_dirs)
else()
    set(component_include_dirs ".")
endif()

# 3. Register the 'Common' component with ESP-IDF
idf_component_register(
    SRCS ${component_srcs}
    INCLUDE_DIRS ${component_include_dirs}
    REQUIRES esp_system log
)
