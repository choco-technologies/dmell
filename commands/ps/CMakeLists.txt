# =====================================================================
#               DMOD ps Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(PS_MODULE_NAME ps)

# Stack size for the ps module
set(DMOD_STACK_SIZE 2048)

# Create the module executable
dmod_add_executable(${PS_MODULE_NAME} ${DMOD_MODULE_VERSION}
    ps.c
)

# dmosi is an external DMOD module (process management) - link its headers
# via dmf-get instead of vendoring its source into our own build.
dmod_link_modules(${PS_MODULE_NAME} dmosi)
