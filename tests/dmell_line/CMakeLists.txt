# =====================================================================
#               dmell_line unit tests (dmod_test.h framework)
# =====================================================================
# Pulls in dmell_bg.c (dmell_run_line reaps background jobs unconditionally)
# and dmell_proc_fake.c specifically (not the real dmell_proc.c) so this test
# never needs a live dmosi backend to be enabled by dmod_loader.
set(DMOD_MODULE_NAME        dmell_line_test)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmell_line_test.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_core/dmell_line.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_core/dmell_cmd.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_core/dmell_redirect.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_core/dmell_bg.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_core/dmell_proc_fake.c
)

target_include_directories(${DMOD_MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/dmell_core
    ${CMAKE_SOURCE_DIR}/include
)

dmod_link_modules(${DMOD_MODULE_NAME} dmosi)

if(DMOD_LOADER)
    add_test(NAME ${DMOD_MODULE_NAME} COMMAND ${DMOD_LOADER} ${DMOD_DMF_DIR}/${DMOD_MODULE_NAME}.dmf)
    set_tests_properties(${DMOD_MODULE_NAME} PROPERTIES ENVIRONMENT "DMOD_DMF_DIR=${DMOD_DMF_DIR}")
endif()
