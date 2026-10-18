dmod_add_test(reboot_test "0.1" reboot_test.c)

if(DMOD_LOADER)
    add_test(NAME reboot_test COMMAND ${DMOD_LOADER} ${DMOD_DMF_DIR}/reboot_test.dmf)
endif()
