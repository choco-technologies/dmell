# =====================================================================
#               DMOD printf Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(PRINTF_MODULE_NAME printf)

# Create the module executable
dmod_add_executable(${PRINTF_MODULE_NAME} ${DMOD_MODULE_VERSION}
    printf.c
)
