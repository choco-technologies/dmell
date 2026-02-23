# =====================================================================
#               DMOD mkdir Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(MKDIR_MODULE_NAME mkdir)

# Stack size for the mkdir module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${MKDIR_MODULE_NAME} ${DMOD_MODULE_VERSION}
    mkdir.c
)
