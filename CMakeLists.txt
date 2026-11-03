cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# Auto-discover all leaf component directories under components/
# Supports both top-level (components/Common) and nested (components/comm/w5500)
# Excludes .template directory from build discovery
# ==============================================================================
file(GLOB_RECURSE component_cmakes LIST_DIRECTORIES false "${CMAKE_CURRENT_LIST_DIR}/components/**/CMakeLists.txt")
list(FILTER component_cmakes EXCLUDE REGEX "/\\.template/")

set(EXTRA_COMPONENT_DIRS "")
foreach(comp_cmake ${component_cmakes})
    get_filename_component(comp_dir ${comp_cmake} DIRECTORY)
    list(APPEND EXTRA_COMPONENT_DIRS ${comp_dir})
endforeach()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ZonalECU-FWv2)
