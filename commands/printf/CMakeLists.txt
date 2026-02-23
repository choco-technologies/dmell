# =====================================================================
#               DMOD printf Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(PRINTF_MODULE_NAME printf)

# Stack size for the printf module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${PRINTF_MODULE_NAME} ${DMOD_MODULE_VERSION}
    printf.c
)
