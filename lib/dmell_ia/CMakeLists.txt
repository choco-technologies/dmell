# =====================================================================
#               DMOD dmell_ia Library
# =====================================================================
set(DMOD_MODULE_NAME        dmell_ia)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)
set(DMOD_STACK_SIZE         1024)

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmell_ia.c
)

target_include_directories(${DMOD_MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(${DMOD_MODULE_NAME}_if INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${DMOD_MODULE_NAME} dmell_cmd_if dmell_script_if)
target_link_libraries(${DMOD_MODULE_NAME}_if INTERFACE dmell_cmd_if)
