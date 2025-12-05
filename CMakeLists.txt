# =====================================================================
#               DMOD RAM File System Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# ======================================================================
#               Unit Tests Option (early to control build behavior)
# ======================================================================
option(DMELL_BUILD_TESTS "Build unit tests" OFF)

# ======================================================================
#               DMOD FFS
# ======================================================================
# Allow version to be passed as a parameter, default to 0.1
if(NOT DEFINED DMOD_MODULE_VERSION)
    set(DMOD_MODULE_VERSION "0.1" CACHE STRING "DMOD module version")
endif()

# ======================================================================
#               Fetch DMOD repository
# ======================================================================
include(FetchContent)
FetchContent_Declare(
    dmod
    GIT_REPOSITORY https://github.com/choco-technologies/dmod.git
    GIT_TAG        develop
)

# ======================================================================
#               DMOD Configuration
# ======================================================================
set(DMOD_MODE "DMOD_MODULE" CACHE STRING "DMOD build mode")
set(DMOD_BUILD_TESTS OFF CACHE BOOL "Build tests")
set(DMOD_BUILD_EXAMPLES OFF CACHE BOOL "Build examples")
set(DMOD_BUILD_TOOLS OFF CACHE BOOL "Build tools")
set(DMOD_BUILD_TEMPLATES OFF CACHE BOOL "Build templates")
set(DMOD_ENABLE_MEMORY_ANALYSIS OFF CACHE BOOL "Enable memory analysis during build")

FetchContent_MakeAvailable(dmod)

project(dmell
    VERSION ${DMOD_MODULE_VERSION}
    DESCRIPTION "DMOD Shell"
    LANGUAGES C CXX)
set(DMOD_DIR ${dmod_SOURCE_DIR} CACHE PATH "DMOD source directory")
    
# ======================================================================
#               Import dmod functions and macros
# ======================================================================
set(DMOD_DIR ${dmod_SOURCE_DIR} CACHE PATH "DMOD source directory")
include(${DMOD_DIR}/paths.cmake)
dmod_setup_external_module()

# ======================================================================
#               Build modules only when not in test mode
# ======================================================================
if(NOT DMELL_BUILD_TESTS)
    # ======================================================================
    #               DMFFS Module Configuration
    # ======================================================================
    # Name of the module 
    set(DMOD_MODULE_NAME        dmell)

    # Version is already set above and used in project()
    # No need to set it again here

    # Author (should be string)
    set(DMOD_AUTHOR_NAME        Patryk Kubiak)

    # Stack size for the module (should be integer)
    set(DMOD_STACK_SIZE         1024)

    #
    #   dmod_add_library - create a library module
    #   it has the same signature as add_library
    #   and can be used in the same way after the creation
    #   (for example, to link libraries)
    #
    dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
        # List of source files - can include C and C++ files
        src/dmell.c
        src/dmell_cmd.c
        src/dmell_line.c
        src/dmell_script.c
        src/dmell_vars.c
        src/dmell_ia.c
        src/dmell_handlers.c
    )

    target_include_directories(${DMOD_MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # ======================================================================
    #               Command Modules
    # ======================================================================
    # Add command modules as subdirectories
    add_subdirectory(commands/cp)
    add_subdirectory(commands/mv)
    add_subdirectory(commands/ls)
    add_subdirectory(commands/cat)
    add_subdirectory(commands/mkdir)
    add_subdirectory(commands/touch)
    add_subdirectory(commands/head)
    add_subdirectory(commands/tail)
    add_subdirectory(commands/grep)
    add_subdirectory(commands/rm)
    add_subdirectory(commands/rmdir)
    add_subdirectory(commands/find)
    add_subdirectory(commands/which)
    add_subdirectory(commands/printf)
endif()

# ======================================================================
#               Unit Tests
# ======================================================================
if(DMELL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
