# =====================================================================
#               DMOD RAM File System Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# ======================================================================
#               Unit Tests Option (early to control build behavior)
# ======================================================================
option(DMELL_BUILD_TESTS "Build unit tests" OFF)

# ======================================================================
#               For VS Code
# ======================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Set DMOD_DIR early
set(DMOD_DIR ${dmod_SOURCE_DIR} CACHE PATH "DMOD source directory")

# Note: dmosi is NOT fetched here. It is an external DMOD module, consumed
# via dmod_link_modules() in lib/dmell_core - the same way any other external
# module dependency is declared, rather than cloning its source into our own
# build.

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
#               dmell_core (shared across dmell instances)
# ======================================================================
# All of dmell's own logic lives in this one library module - see
# lib/dmell_core/CMakeLists.txt for why it isn't split further right now.
# dmod_add_library()'s link step (-nostdlib, custom module.ld, ...) only
# works under DMOD_MODE=DMOD_MODULE, same as the dmell executable and
# command modules below - dmod_test.h test modules (tests/) share this same
# build, they are not a separate host-native build.
#
# DMELL_PROC_FAKE controls which process backend dmell_core's dmell_proc.c
# calls into: real dmosi requires a live backend to be *enabled* by whatever
# loads the module (see the split plan), which the generic dmod_loader used
# to run dmod_test.h test modules does not provide. Tied to DMELL_BUILD_TESTS
# so test builds get a simulated backend instead.
set(DMELL_PROC_FAKE ${DMELL_BUILD_TESTS})

if(DMOD_MODE STREQUAL "DMOD_MODULE")
    add_subdirectory(lib/dmell_core)
endif()

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
    set(DMOD_STACK_SIZE         4096)

    #
    #   dmod_add_library - create a library module
    #   it has the same signature as add_library
    #   and can be used in the same way after the creation
    #   (for example, to link libraries)
    #
    dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
        # List of source files - can include C and C++ files
        src/dmell.c
    )

    # dmell only ever talks to the dmell_core facade - everything else
    # (dmell_ia, dmell_script, dmell_handlers, ...) is dmell_core's own
    # required-module dependency, not dmell's. That is deliberate: it is the
    # one place that will decide what to load lazily once dmod supports it,
    # without dmell's own executable ever needing to change again.
    target_link_libraries(${DMOD_MODULE_NAME} dmell_core_if)

    # ======================================================================
    #               Command Modules
    # ======================================================================
    # Add command modules as subdirectories
    add_subdirectory(commands/cp)
    add_subdirectory(commands/mv)
    add_subdirectory(commands/ls)
    add_subdirectory(commands/cat)
    add_subdirectory(commands/catini)
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
    add_subdirectory(commands/ps)
endif()

# ======================================================================
#               Unit Tests
# ======================================================================
if(DMELL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
