# ======================================================================
#               DMELL Unit Tests
# ======================================================================
# Tests are real DMOD Application test modules built with dmod_add_test()
# and dmod_test.h (DMOD_TEST_STEP), linking against the actual dmell_*
# library modules through their _if targets - the same cross-module
# mechanism dmell's own modules use to call each other. Each test module
# is run through dmod_loader, exactly like any other DMOD Application
# module; its exit code is the number of failed steps (0 = all passed).
#
# This runs under the same DMOD_MODE=DMOD_MODULE build as the rest of the
# project (see the top-level CMakeLists.txt) - dmod_add_test/dmod_add_library
# share the same module-linking model, so no separate "system mode" build is
# needed just to compile tests.

find_program(DMOD_LOADER dmod_loader)
if(NOT DMOD_LOADER)
    message(WARNING "dmod_loader not found - tests will build but ctest will not be able to run them")
endif()

add_subdirectory(dmell_vars)
add_subdirectory(dmell_cmd)
add_subdirectory(dmell_line)
