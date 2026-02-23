# =====================================================================
#               DMOD cat Command Module
# =====================================================================
cmake_minimum_required(VERSION 3.18)

# Module name
set(CAT_MODULE_NAME cat)

# Stack size for the cat module
set(DMOD_STACK_SIZE 512)

# Create the module executable
dmod_add_executable(${CAT_MODULE_NAME} ${DMOD_MODULE_VERSION}
    cat.c
)
