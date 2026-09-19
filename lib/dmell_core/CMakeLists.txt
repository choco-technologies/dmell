# =====================================================================
#               DMOD dmell_core Library
# =====================================================================
# Everything dmell does used to be split into separate dmod library modules
# (dmell_vars, dmell_cmd, dmell_bg, dmell_line, dmell_script, dmell_handlers,
# dmell_ia, dmell_proc) so each piece could eventually be loaded lazily.
# Merged back into this single module: dmod has no lazy-loading support yet,
# so that benefit doesn't exist today, while the per-module fixed cost
# (header/footer + duplicated cross-module signature strings, ~300-500B per
# module) was real and measured at ~3kB+ across ten modules. See the split
# plan for the numbers. dmell (the executable) still only ever links against
# this one module, unchanged.
set(DMOD_MODULE_NAME        dmell_core)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)
set(DMOD_STACK_SIZE         4096)

if(NOT DEFINED DMELL_PROC_FAKE)
    set(DMELL_PROC_FAKE OFF)
endif()

if(DMELL_PROC_FAKE)
    set(DMELL_PROC_SOURCE dmell_proc_fake.c)
else()
    set(DMELL_PROC_SOURCE dmell_proc.c)
endif()

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmell_vars.c
    dmell_redirect.c
    ${DMELL_PROC_SOURCE}
    dmell_cmd.c
    dmell_bg.c
    dmell_line.c
    dmell_script.c
    dmell_handlers.c
    dmell_ia.c
    dmell_core.c
    dmell_runtime_stubs.c
)

target_include_directories(${DMOD_MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(${DMOD_MODULE_NAME}_if INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

# dmosi is an external DMOD module (process management) - link its headers
# via dmf-get instead of vendoring its source into our own build. Needed in
# both DMELL_PROC_FAKE variants: dmell_proc.h's declarations use dmosi's
# types (dmosi_process_t etc.) regardless of whether the real or the fake .c
# actually calls into dmosi's functions.
dmod_link_modules(${DMOD_MODULE_NAME} dmosi)
