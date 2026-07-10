# =====================================================================
#               dmell_vars unit tests (dmod_test.h framework)
# =====================================================================
set(DMOD_MODULE_NAME        dmell_vars_test)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmell_vars_test.c
)

target_link_libraries(${DMOD_MODULE_NAME} dmell_vars_if)

if(DMOD_LOADER)
    add_test(NAME ${DMOD_MODULE_NAME} COMMAND ${DMOD_LOADER} ${DMOD_DMF_DIR}/${DMOD_MODULE_NAME}.dmf)
    # dmod_loader resolves required modules (dmell_vars) by name via the
    # DMOD_DMF_DIR *environment* variable, independently of the CMake
    # variable of the same name used for build output paths above - override
    # it for the test run so it looks in this build's own dmf/ output
    # instead of whatever DMOD_DMF_DIR happens to be set to in the ambient
    # shell environment.
    set_tests_properties(${DMOD_MODULE_NAME} PROPERTIES ENVIRONMENT "DMOD_DMF_DIR=${DMOD_DMF_DIR}")
endif()
