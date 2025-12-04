# ======================================================================
#               DMELL Unit Tests
# ======================================================================

# ===========================================================================
#   Fetch the Google Test framework
# ===========================================================================
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# ===========================================================================
#                       Test source files
# ===========================================================================
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests_dmell_vars.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests_dmell_cmd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests_dmell_line.cpp
)

# ===========================================================================
#                       Source files under test
# ===========================================================================
set(DMELL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/dmell_vars.c
    ${CMAKE_SOURCE_DIR}/src/dmell_cmd.c
    ${CMAKE_SOURCE_DIR}/src/dmell_line.c
)

# ===========================================================================
#                       Build each test as separate executable
# ===========================================================================
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    
    add_executable(${test_name} 
        ${test_src} 
        ${CMAKE_CURRENT_SOURCE_DIR}/ut_main.cpp
        ${DMELL_SOURCES}
    )
    
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${DMOD_DIR}/inc
        ${CMAKE_BINARY_DIR}
    )
    
    target_link_libraries(${test_name} 
        gtest 
        gtest_main 
        gmock
        dmod_inc
        dmod_common
        dmod_system
    )
    
    # Add test to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # Add to test files list
    list(APPEND TEST_EXECUTABLES ${test_name})
endforeach()

# ===========================================================================
#                       Custom target to run all tests
# ===========================================================================
add_custom_target(dmell_tests ALL
    DEPENDS ${TEST_EXECUTABLES}
)

add_custom_target(run_tests 
    COMMAND ctest --output-on-failure
    DEPENDS ${TEST_EXECUTABLES}
)
