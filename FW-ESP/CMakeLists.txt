cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# Auto-discover all leaf component directories under comp/
# Supports both top-level (comp/Common) and nested (comp/Comm/W5500)
# Excludes .template directory from build discovery
# ==============================================================================
file(GLOB_RECURSE component_cmakes LIST_DIRECTORIES false "${CMAKE_CURRENT_LIST_DIR}/comp/**/CMakeLists.txt")
list(FILTER component_cmakes EXCLUDE REGEX "/\\.template/")

set(EXTRA_COMPONENT_DIRS "")
foreach(comp_cmake ${component_cmakes})
    get_filename_component(comp_dir ${comp_cmake} DIRECTORY)
    list(APPEND EXTRA_COMPONENT_DIRS ${comp_dir})
endforeach()

# ==============================================================================
# Auto-install Git pre-commit hook on fresh clones (for any developer laptop)
# ==============================================================================
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git/hooks" AND
   EXISTS "${CMAKE_CURRENT_LIST_DIR}/tools/pre-commit-hook.sh" AND
   NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git/hooks/pre-commit")
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/tools/pre-commit-hook.sh"
         DESTINATION "${CMAKE_CURRENT_LIST_DIR}/.git/hooks"
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    file(RENAME "${CMAKE_CURRENT_LIST_DIR}/.git/hooks/pre-commit-hook.sh"
                "${CMAKE_CURRENT_LIST_DIR}/.git/hooks/pre-commit")
    message(STATUS "[ZonalECU-FWv2] Installed Git pre-commit hook (.git/hooks/pre-commit)")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Enforce strict compiler warnings as errors (Layer 2: Build Gatekeeper)
idf_build_set_property(COMPILE_OPTIONS "-Werror=unused-function" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Werror=unused-variable" APPEND)

project(ZonalECU-FWv2)
