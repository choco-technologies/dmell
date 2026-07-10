# =====================================================================
#               dmell_cmd unit tests (dmod_test.h framework)
# =====================================================================
set(DMOD_MODULE_NAME        dmell_cmd_test)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmell_cmd_test.c
)

target_link_libraries(${DMOD_MODULE_NAME} dmell_cmd_if)

if(DMOD_LOADER)
    add_test(NAME ${DMOD_MODULE_NAME} COMMAND ${DMOD_LOADER} ${DMOD_DMF_DIR}/${DMOD_MODULE_NAME}.dmf)
    set_tests_properties(${DMOD_MODULE_NAME} PROPERTIES ENVIRONMENT "DMOD_DMF_DIR=${DMOD_DMF_DIR}")
endif()
