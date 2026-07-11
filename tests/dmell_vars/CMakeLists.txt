# =====================================================================
#               dmell_vars unit tests (dmod_test.h framework)
# =====================================================================
# Compiles the relevant dmell_core source directly (now plain C, no
# cross-module macros - dmell_core merged everything back into one module,
# see lib/dmell_core/CMakeLists.txt), so this test never touches the dmod
# loader's required-module resolution at all for these functions.
set(DMOD_MODULE_NAME        dmell_vars_test)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmell_vars_test.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_core/dmell_vars.c
)

target_include_directories(${DMOD_MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/dmell_core
    ${CMAKE_SOURCE_DIR}/include
)

if(DMOD_LOADER)
    add_test(NAME ${DMOD_MODULE_NAME} COMMAND ${DMOD_LOADER} ${DMOD_DMF_DIR}/${DMOD_MODULE_NAME}.dmf)
    set_tests_properties(${DMOD_MODULE_NAME} PROPERTIES ENVIRONMENT "DMOD_DMF_DIR=${DMOD_DMF_DIR}")
endif()
