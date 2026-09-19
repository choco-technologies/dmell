# =====================================================================
#               DMOD ps Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(PS_MODULE_NAME ps)

# Stack size for the ps module
set(DMOD_STACK_SIZE 2048)

# Create the module executable
# ps.c's format_time() does uint64_t division/modulo (runtime_ms is a
# millisecond count that can span far more than 2^32 ms) - see
# lib/dmell_runtime_stubs/dmell_runtime_stubs.c for why that needs a
# raise() stub to link on this toolchain.
dmod_add_executable(${PS_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ps.c
    ${CMAKE_SOURCE_DIR}/lib/dmell_runtime_stubs/dmell_runtime_stubs.c
)

# dmosi is an external DMOD module (process management) - link its headers
# via dmf-get instead of vendoring its source into our own build.
dmod_link_modules(${PS_MODULE_NAME} dmosi)
