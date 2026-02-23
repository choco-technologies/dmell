# =====================================================================
#               DMOD grep Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(GREP_MODULE_NAME grep)

# Stack size for the grep module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${GREP_MODULE_NAME} ${DMOD_MODULE_VERSION}
    grep.c
)
