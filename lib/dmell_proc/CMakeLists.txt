# =====================================================================
#               DMOD dmell_proc Library
# =====================================================================
# Isolates every dmosi process-API call in dmell behind one small module, so
# it alone needs dmosi - and so it can be swapped for a simulated backend in
# test builds (DMELL_PROC_FAKE, set from DMELL_BUILD_TESTS below) instead of
# requiring a real dmosi implementation to be enabled just to load a test
# module via dmod_loader.
set(DMOD_MODULE_NAME        dmell_proc)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        Patryk Kubiak)
set(DMOD_STACK_SIZE         1024)

if(NOT DEFINED DMELL_PROC_FAKE)
    set(DMELL_PROC_FAKE OFF)
endif()

if(DMELL_PROC_FAKE)
    set(DMELL_PROC_SOURCE dmell_proc_fake.c)
else()
    set(DMELL_PROC_SOURCE dmell_proc.c)
endif()

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ${DMELL_PROC_SOURCE}
)

target_include_directories(${DMOD_MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(${DMOD_MODULE_NAME}_if INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

# dmosi is an external DMOD module (process management) - link its headers
# via dmf-get instead of vendoring its source into our own build. Needed in
# both variants: dmell_proc.h's declarations use dmosi's types (dmosi_process_t
# etc.) regardless of whether the real or the fake .c actually calls into
# dmosi's functions, and consumers (dmell_bg, dmell_handlers) see those same
# declarations through dmell_proc_if.
dmod_link_modules(${DMOD_MODULE_NAME} dmosi)
dmod_link_modules(${DMOD_MODULE_NAME}_if INTERFACE dmosi)
