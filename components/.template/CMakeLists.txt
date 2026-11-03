# ==============================================================================
# ESP-IDF COMPONENT CMAKE TUTORIAL & TEMPLATE
# ==============================================================================
# This file serves as both an educational guide to CMake in ESP-IDF and a 
# production-ready blueprint for creating new components.
#
# ESP-IDF Architecture Note:
# Every subdirectory containing a CMakeLists.txt is treated as an independent 
# component library target. The component name defaults to the folder name 
# (e.g., folder 'w5500' -> component name 'w5500', target '___idf_w5500').
# ==============================================================================


# ==============================================================================
# SECTION 1: SOURCE FILE COLLECTION (SRCS)
# ==============================================================================
# Command: file(GLOB_RECURSE <output_variable> <globbing_expressions>...)
#
# How it works:
# - GLOB_RECURSE: Traverses all subdirectories recursively starting from the
#   given path to find any files matching the wildcard pattern (e.g. *.c, *.cpp).
# - ${CMAKE_CURRENT_LIST_DIR}: Built-in CMake variable holding the absolute path
#   to the directory containing THIS CMakeLists.txt file.
# - component_srcs: A list variable that will store the paths of all discovered files.
#
# Option A (Auto-discovery / Recursive Scan):
file(GLOB_RECURSE component_srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    # "${CMAKE_CURRENT_LIST_DIR}/*.cpp"  # Uncomment if C++ sources are present
)

# Option B (Explicit File Listing - Recommended for large production modules):
# If you prefer strict control over which files compile, use 'set':
#   command: set(<variable_name> <value1> <value2> ...)
# Example:
# set(component_srcs
#     "src/driver.c"
#     "src/hal.c"
# )


# ==============================================================================
# SECTION 2: HEADER DIRECTORY DISCOVERY (INCLUDE_DIRS)
# ==============================================================================
# To allow code to #include header files, the compiler needs -I<directory> flags.
#
# Step 2.1: Find all header files (.h) recursively
file(GLOB_RECURSE component_headers
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

# Step 2.2: Extract the directory part of each header file
# - foreach(<item_var> ${<list_var>}) ... endforeach(): Iterates over each item.
# - get_filename_component(<out_var> <input_path> DIRECTORY): Extracts only the 
#   directory portion of a full file path (e.g. "/path/to/module/header.h" -> "/path/to/module").
# - list(APPEND <list_name> <value>): Adds elements to the end of the list.
set(component_include_dirs "")
foreach(header ${component_headers})
    get_filename_component(header_dir ${header} DIRECTORY)
    list(APPEND component_include_dirs ${header_dir})
endforeach()

# Step 2.3: Add the parent directory (components/) to include paths
# - "${CMAKE_CURRENT_LIST_DIR}/..": Points to the parent directory.
# - Benefit: Enables consumers to include headers with a module namespace prefix:
#   Example: #include "Common/Error.h" or #include "w5500/w5500_driver.h"
list(APPEND component_include_dirs "${CMAKE_CURRENT_LIST_DIR}/..")

# Step 2.4: Clean up include list (remove duplicates and sort)
# - list(REMOVE_DUPLICATES <list_name>): Removes duplicate directories from the list.
# - list(SORT <list_name>): Sorts the list alphabetically.
# - if(<condition>) ... else() ... endif(): Standard CMake conditional branch.
if(component_include_dirs)
    list(REMOVE_DUPLICATES component_include_dirs)
    list(SORT component_include_dirs)
else()
    set(component_include_dirs ".")
endif()


# ==============================================================================
# SECTION 3: COMPONENT REGISTRATION (idf_component_register)
# ==============================================================================
# idf_component_register() is the core ESP-IDF function that configures this 
# component as a static library (.a) and resolves dependencies.
#
# Argument Reference Guide:
# +--------------------+-------------------------------------------------------+
# | Argument           | Description & Practical Usage                         |
# +--------------------+-------------------------------------------------------+
# | SRCS               | Source files (.c, .cpp, .S) to compile into this      |
# |                    | component's static library.                           |
# |                    | Example: ${component_srcs}                            |
# +--------------------+-------------------------------------------------------+
# | INCLUDE_DIRS       | PUBLIC include directories. Exported to this          |
# |                    | component AND any other component that REQUIRES it.   |
# |                    | Example: ${component_include_dirs}                    |
# +--------------------+-------------------------------------------------------+
# | PRIV_INCLUDE_DIRS  | PRIVATE include directories. Used ONLY when compiling |
# |                    | files inside this component. Hidden from consumers.   |
# |                    | Example: "private_include"                            |
# +--------------------+-------------------------------------------------------+
# | REQUIRES           | PUBLIC component dependencies.                        |
# |                    | - This component needs them.                          |
# |                    | - Any component depending on THIS component will also |
# |                    |   transitively inherit these dependencies.            |
# |                    | Example: Common (shared error codes/macros)           |
# +--------------------+-------------------------------------------------------+
# | PRIV_REQUIRES      | PRIVATE component dependencies.                       |
# |                    | - Used ONLY inside this component's .c source files.  |
# |                    | - NOT propagated to outside callers.                  |
# |                    | Example: esp_driver_spi (only internal C code speaks  |
# |                    | to hardware SPI; outside callers just call w5500 API).|
# +--------------------+-------------------------------------------------------+
idf_component_register(
    SRCS 
        ${component_srcs}

    INCLUDE_DIRS 
        ${component_include_dirs}

    # PRIV_INCLUDE_DIRS
    #     "private_include"

    REQUIRES 
        Common
        esp_system 
        log

    # PRIV_REQUIRES 
    #     esp_driver_spi   # Hardware SPI master driver
    #     freertos         # Tasks, queues, semaphores
    #     esp_timer        # High resolution timers
)


# ==============================================================================
# SECTION 4: ADVANCED TARGET PROPERTIES (OPTIONAL)
# ==============================================================================
# ${COMPONENT_LIB}: Built-in ESP-IDF variable holding the CMake target name of 
# this component library (e.g. '___idf_Common' or '___idf_w5500').
#
# 1. Custom Compiler Flags for this component only:
# target_compile_options(${COMPONENT_LIB} PRIVATE 
#     -Wno-unused-variable 
#     -Wno-unused-function
# )
#
# 2. Custom Preprocessor Definitions (-DMACRO=VALUE) for this component only:
# target_compile_definitions(${COMPONENT_LIB} PRIVATE 
#     MODULE_DEBUG_ENABLED=1
# )
