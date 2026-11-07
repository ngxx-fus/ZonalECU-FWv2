# ==============================================================================
# Component: SomeIp
# AUTOSAR SOME/IP protocol (PRS R22-11): Core (types, header, endian, session,
# check) and Serialization. Sub-folders are plain source folders of this single
# component, so they must NOT have their own CMakeLists.txt.
# ==============================================================================

# 1. Collect all C source files (Core/, Serialization/, ...)
file(GLOB_RECURSE component_srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

# 2. Every folder that holds a header becomes an include path, so internal
#    files keep using flat includes (e.g. #include "SomeIp_Types.h")
file(GLOB_RECURSE component_headers
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

set(component_include_dirs "")
foreach(header ${component_headers})
    get_filename_component(header_dir ${header} DIRECTORY)
    list(APPEND component_include_dirs ${header_dir})
endforeach()

# Parent directory (components/) so consumers can use: #include "SomeIp/SomeIp.h"
list(APPEND component_include_dirs "${CMAKE_CURRENT_LIST_DIR}/..")

if(component_include_dirs)
    list(REMOVE_DUPLICATES component_include_dirs)
    list(SORT component_include_dirs)
else()
    set(component_include_dirs ".")
endif()

# 3. Register the 'SomeIp' component (pure protocol logic, no hardware access)
idf_component_register(
    SRCS ${component_srcs}
    INCLUDE_DIRS ${component_include_dirs}
    REQUIRES Common
)
