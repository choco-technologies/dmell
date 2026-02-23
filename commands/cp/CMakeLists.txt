# =====================================================================
#               DMOD cp Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(CP_MODULE_NAME cp)

# Stack size for the cp module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${CP_MODULE_NAME} ${DMOD_MODULE_VERSION}
    cp.c
)
